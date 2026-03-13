#include "ServerManager.hpp"

/**
 * @brief Maps status code to reason phrase for fallback/plain responses.
 */
static std::string getReasonPhrase(int statusCode)
{
	switch (statusCode)
	{
		case 200: return "OK";
		case 400: return "Bad Request";
		case 431: return "Request Header Fields Too Large";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "Error";
	}
}

/**
 * @brief Builds a minimal HTTP response when no specialized responder is used.
 * @param statusCode HTTP status code.
 * @param keepAlive Whether to keep connection open.
 * @param explicitBody Optional response body override.
 * @return Serialized HTTP response text.
 */
static std::string buildDefaultResponse(int status, bool keepAlive, const std::string &body, const std::string &version)
{
	//Person 3 ownership: replace this fallback with the final Response engine serializer.
	// If no explicit body was prepared by higher-level logic, use a tiny
	// fallback body derived from the status text so the client still gets
	// a valid human-readable response.
	std::string responseBody = body;
	if (responseBody.empty())
		responseBody = getReasonPhrase(status) + "\n";
	std::ostringstream oss;
	oss << version << " " << status << " " << getReasonPhrase(status) << "\r\n";
	oss << "Content-Type: text/plain\r\n";
	oss << "Content-Length: " << responseBody.size() << "\r\n";
	oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
	oss << "\r\n";
	oss << responseBody;
	return oss.str();
}


/**
 * @brief Advances one client state-machine step based on current state.
 * @param client Client session.
 * @param server Owning server configuration (used for body-size and future routing hooks).
 */
void ServerManager::handleClientRequest(ClientSession &client, ServerConfig &server)
{
	switch (client.state)
	{
		case CLOSING:
			printLog("ℹ️ Client already marked closing", BYEL);
			closeClientSocket(client);
			return;
		case READING:
			readClientRequest(client, static_cast<size_t>(server.getMaxBodySize()));
			if (client.state == WRITING)
			{
				// readClientRequest() can decide an immediate error response
				// (e.g. unsupported chunked, oversized headers/body).
				modClientEpoll(client, EPOLLOUT);
				break;
			}
			if (client.state != PROCESSING)
				break;
			// Full request received: continue directly into the processing step
			// during the same event-loop pass instead of waiting for another event.
			//fall-through
		case PROCESSING:
			parseClientRequest(client, server);
			if (client.state == WRITING)
				// EPOLLOUT means "wake me when this fd can be written without blocking".
				// Once a response is ready, we switch from read readiness to write readiness.
				modClientEpoll(client, EPOLLOUT);
			break;
		case WRITING:
			sendClientResponse(client);
			break;
		case IDLE:
		default:
			break;
	}
	client.lastActive = time(0);
}

/**
 * @brief Parses buffered request text and updates session response metadata.
 * @param client Client session.
 * @param server Owning server configuration for Person 2/3 routing/resource phases.
 */
void ServerManager::parseClientRequest(ClientSession &client, ServerConfig &server)
{
	// Parse only the first complete request currently in readBuffer.
	// Any trailing bytes (possible next pipelined request) are preserved.
	size_t requestSize = std::string::npos;
	// Find end of headers for the current request frame.
	size_t headerEnd = client.readBuffer.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
	{
		size_t headerSize = headerEnd + 4;
		// Guard bounds before computing total request bytes.
		if (headerSize <= client.readBuffer.size()
			&& client.contentLength <= client.readBuffer.size() - headerSize)
			// Exact bytes belonging to this request only.
			requestSize = headerSize + client.contentLength;
	}

	// `requestBuffer` is what we parse now; `remainingBuffer` is queued for next cycle.
	std::string requestBuffer = client.readBuffer;
	std::string remainingBuffer;
	if (requestSize != std::string::npos && requestSize <= client.readBuffer.size())
	{
		requestBuffer = client.readBuffer.substr(0, requestSize);
		if (requestSize < client.readBuffer.size())
			remainingBuffer = client.readBuffer.substr(requestSize);
	}

	client.request = Request();
	Request &request = client.request;
	if (!request.parseRequest(requestBuffer, client.contentLength))
	{
		client.writeBuffer.clear();
		client.path = "";
		client.method = NONE;
		client.status = request.getStatus();
		printLog("⚠️ Malformed HTTP request handled", YEL);
		client.keepAlive = false;
		client.state = WRITING;
		remainingBuffer.clear();
	}
	else
	{
		// Person 2 hook: receive parsed Request + current ServerConfig and decide
		// routing/config result (best location, method validation, effective path, status).
		// For now processClientRequest() copies the parsed metadata into transport fields.
		processClientRequest(client, request, server);
		if (!client.keepAlive)
			remainingBuffer.clear();
	}

	// Keep only leftover bytes that belong to future requests.
	client.readBuffer = remainingBuffer;
	client.contentLength = 0;
	return;
}

/**
 * @brief Copies parsed request metadata into session transport/response fields.
 * @param client Client session.
 * @param request Parsed request object.
 * @param server Owning server configuration (available for Person 2/3 decisions).
 */
void ServerManager::processClientRequest(ClientSession &client, Request &request, ServerConfig &server)
{
	// Person 2 hook: use `server` + `request` to compute final route,
	// location match, allowed methods, and resolved filesystem path.
	// Person 3 hook: use `server` error pages/root/indexes to build final body.
	(void)server;
	client.method = request.getMethod();
	client.path = request.getPath();
	client.status = request.getStatus();
	client.isRedirection = request.isRedirect();
	client.version = request.getVersion();
	// Determine keep-alive behavior from HTTP version + Connection header:
	// - HTTP/1.0 defaults to close unless Connection: keep-alive
	// - HTTP/1.1 defaults to keep-alive unless Connection: close
	std::string clientHeader = toLower(request.getHeader("connection"));
	if (client.version == "HTTP/1.0")
		client.keepAlive = (clientHeader == "keep-alive");
	else
		client.keepAlive = (clientHeader != "close");

	// Placeholder response source until the real resource engine exists.
	if (request.isAutoIndex())
		// Person 3 hook: replace this placeholder body source with final
		// resource engine output (file content/error page/rendered directory).
		client.responseStr = request.getAutoIndexPath();
	else
		client.responseStr.clear();
	// Parsing + request processing is done; next step is writing a response.
	client.state = WRITING;
}

/**
 * @brief Sends response bytes and handles keep-alive reset/close decisions.
 * @param client Client session.
 * @param epollFd Epoll instance descriptor (kept for interface consistency).
 */
void ServerManager::sendClientResponse(ClientSession &client)
{
	if (client.fd < 0)
	{
		client.state = CLOSING;
		return;
	}

	if (client.writeBuffer.empty())
		// Person 3 hook: build Response object + headers/body here, then
		// serialize to HTTP text (status line, Content-Length, Connection).
		client.writeBuffer = buildDefaultResponse(client.status, client.keepAlive, client.responseStr, client.version);

	// send() starts at writeBuffer + totalSent so partially sent responses can resume.
	ssize_t sentBytes = send(client.fd, client.writeBuffer.c_str() + client.totalSent,
		client.writeBuffer.size() - client.totalSent, MSG_NOSIGNAL);
	if (sentBytes < 0)
	{
		//same thing as the recv problem. Give one change to try again, then set as closed
		client.ioFailures++;
		if (client.ioFailures < 2)
		{
			client.state = WRITING; // keep EPOLLOUT; give the buffer one chance to drain
			return;
		}
		printLog("🚨 send() failed twice consecutively — closing connection", RED);
		client.state = CLOSING;
		return;
	}
	client.ioFailures = 0; // successful send: reset the consecutive-failure counter

	client.totalSent += static_cast<size_t>(sentBytes);
	// If not all bytes were sent this time, keep EPOLLOUT and continue later.
	if (client.totalSent < client.writeBuffer.size())
		return;

	client.writeBuffer.clear();
	client.totalSent = 0;
	client.responseStr.clear();
	client.headersSent = true;

	if (client.keepAlive)
	{
		client.state = READING;
		client.method = NONE;
		client.status = 200;
		client.contentLength = 0;
		client.ioFailures = 0; // reset for the next request on this keep-alive connection
		// Switch back to EPOLLIN so epoll wakes us when the next request arrives
		// on this keep-alive connection.
		modClientEpoll(client, EPOLLIN);
	}
	else
		client.state = CLOSING;
}
