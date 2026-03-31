#include "ServerManager.hpp"
#include "../routing/ConfigResolved.hpp"
#include "../Response/ResponseBuilder.hpp"

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
			sendClientResponse(client, server);
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
	// Header size limit (RFC 6585, 431): 8KB is common, adjust as needed
	const size_t MAX_HEADER_SIZE = 8192;
	if (headerEnd != std::string::npos)
	{
		size_t headerSize = headerEnd + 4;
		if (headerSize > MAX_HEADER_SIZE) {
			client.writeBuffer.clear();
			client.path = "";
			client.method = NONE;
			client.status = 431;
			printLog("🚨 Oversized headers: 431 Request Header Fields Too Large", RED);
			client.keepAlive = false;
			client.state = WRITING;
			client.readBuffer.clear();
			client.contentLength = 0;
			client.version = "HTTP/1.1";
			// Use ResponseBuilder for error response
			Request errorRequest;
			errorRequest.setStatus(431);
			errorRequest.setVersion(client.version);
			ConfigResolved config(errorRequest, server);
			ResponseBuilder rb;
			client.writeBuffer = rb.returnGenericErrorResponse(431, errorRequest, config);
			modClientEpoll(client, EPOLLOUT); // Ensure response is sent
			return;
		}
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
				// Use ResponseBuilder for error response
				ConfigResolved config(request, server);
				ResponseBuilder rb;
				client.writeBuffer = rb.returnGenericErrorResponse(request.getStatus(), request, config);
			} else
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
	ConfigResolved routing(request, server);
	// Person 3 hook: use `server` error pages/root/indexes to build final body.
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
	//CGI
	if (request.isCgi(routing))
	{
		startCgi(client, request.getCgiFullPath(), request.getCgiInterpreter(), server);
		return;
	}

	// else {
	// 	// Handle error: interpreter not found
	// 	client.status = 501;
	// 	client.keepAlive = false;
	// 	client.state = WRITING;
	// 	ResponseBuilder rb;
	// 	ConfigResolved config(request, server);
	// 	client.writeBuffer = rb.returnGenericErrorResponse(501, request, config);
	// 	client.totalSent = 0;
	// 	modClientEpoll(client, EPOLLOUT);
	// }
	// return;
	// }
	//NORMAL
	ResponseBuilder rb;
	client.writeBuffer = rb.returnResponse(request, routing, client.keepAlive);
	client.totalSent = 0;
	client.responseStr.clear(); // optional, but avoids mixing old placeholder paths
	client.state = WRITING;
}


/**
 * @brief Sends response bytes and handles keep-alive reset/close decisions.
 * @param client Client session.
 */
void ServerManager::sendClientResponse(ClientSession &client, ServerConfig &server)
{
	if (client.fd < 0)
	{
		client.state = CLOSING;
		return;
	}

	if (client.writeBuffer.empty()) {
		// Use ResponseBuilder for error responses if writeBuffer is empty
		Request errorRequest;
		errorRequest.setStatus(client.status);
		errorRequest.setVersion(client.version);
		ConfigResolved config(errorRequest, server);
		ResponseBuilder rb;
		client.writeBuffer = rb.returnGenericErrorResponse(client.status, errorRequest, config);
	}

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
		client.sent100Continue = false;  // reset for next request
		client.ioFailures = 0; // reset for the next request on this keep-alive connection
		// Switch back to EPOLLIN so epoll wakes us when the next request arrives
		// on this keep-alive connection.
		modClientEpoll(client, EPOLLIN);
	}
	else
		client.state = CLOSING;
}
