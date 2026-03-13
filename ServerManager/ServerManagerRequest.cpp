#include "ServerManager.hpp"

static const size_t MAX_HEADER_SIZE = 8192;

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
 * @brief Checks if a given header line contains a specific token.
 */
static bool hasHeaderToken(const std::string &headersLower, const std::string &headerName, const std::string &token)
{
	// Scan one header line at a time so matches are anchored to real line starts.
	for (size_t lineStart = 0; lineStart < headersLower.size(); )
	{
		// Find the end of the current header line.
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			// Last line in the buffer may not have trailing CRLF.
			lineEnd = headersLower.size();

		// Accept only exact header-name match at the beginning of this line.
		// This prevents spoofing via request target text or other header values.
		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, headerName.size(), headerName) == 0)
		{
			// Search token only inside the matched header line.
			return headersLower.substr(lineStart, lineEnd - lineStart).find(token) != std::string::npos;
		}

		if (lineEnd == headersLower.size())
			// Reached the last line.
			break;
		// Move to the next line (skip "\r\n").
		lineStart = lineEnd + 2;
	}
	return false;
}

/**
 * @brief Parses `Content-Length` from normalized headers when present.
 * @param headersLower Lowercased header block.
 * @param outContentLength Output parsed length.
 * @return true on success or when header is absent, false on invalid value.
 */
static bool parseContentLengthValue(const std::string &headersLower, size_t &outContentLength)
{
	const std::string header = "content-length:";
	// `lineStart` walks line by line; `start/end` delimit the numeric slice.
	size_t lineStart = 0;
	size_t start = std::string::npos;
	size_t end = std::string::npos;

	// Find the Content-Length header only when it appears at a line start.
	for (; lineStart < headersLower.size(); )
	{
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = headersLower.size();

		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, header.length(), header) == 0)
		{
			// Number starts right after "content-length:" and ends at line end.
			start = lineStart + header.length();
			end = lineEnd;
			break;
		}

		if (lineEnd == headersLower.size())
			break;
		lineStart = lineEnd + 2;
	}

	if (start == std::string::npos)
		// Header absent is not automatically an error here; caller decides.
		return true;

	// Skip optional whitespace before the number.
	while (start < end && (headersLower[start] == ' ' || headersLower[start] == '\t'))
		++start;

	// Trim optional trailing spaces/tabs after the numeric value.
	while (end > start && (headersLower[end - 1] == ' ' || headersLower[end - 1] == '\t'))
		--end;

	// Empty value after trimming is invalid (e.g. "content-length:   ").
	if (end <= start)
		return false;

	// Isolate the raw Content-Length token to parse.
	std::string valueStr = headersLower.substr(start, end - start);
	long value;
	try
	{
		// Shared parser validates range, format, and trailing characters.
		value = strToLong(valueStr);
	}
	catch (const std::exception &)
	{
		// Convert parsing exceptions into this function's bool error contract.
		return false;
	}

	if (value < 0)
		return false;

	// Ensure the parsed value can fit in size_t before casting.
	if (static_cast<unsigned long>(value) > std::numeric_limits<size_t>::max())
		return false;

	// Store validated Content-Length.
	outContentLength = static_cast<size_t>(value);
	return true;
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
 * @brief Reads request bytes and transitions session into processing when complete.
 * @param client Client session.
 * @param maxUploadSize Max accepted body size for this endpoint.
 */
void ServerManager::readClientRequest(ClientSession &client, size_t maxUploadSize)
{
	if (client.fd < 0)
	{
		printLog("🚨 No socket available", RED);
		client.state = CLOSING;
		return;
	}

	// Temporary stack buffer used for one recv() call.
	char buffer[BUFFER_SIZE];
	int readBytes = recv(client.fd, buffer, sizeof(buffer), 0);
	// recv() reads up to sizeof(buffer) bytes from socket _fd into buffer.
	// The last argument is 0, which means recv() is called with no special flags.
	if (readBytes < 0)
	{
		// Cannot inspect errno, so we cannot tell whether this is EAGAIN/EWOULDBLOCK/EINTR
		//   First failure  → stay READING, return. EPOLLIN stays armed; retry next event.
		//   Second consecutive failure, set state as CLOSING.
		client.ioFailures++;
		if (client.ioFailures < 2)
		{
			client.state = READING; // keep EPOLLIN; give the socket one more chance
			return;
		}
		printLog("🚨 recv() failed twice consecutively — closing connection", RED);
		client.state = CLOSING;
		return;
	}
	client.ioFailures = 0; // successful read: reset the consecutive-failure counter

	if (readBytes == 0)
	{
		// recv() == 0 means the peer performed an orderly shutdown.
		printLog("ℹ️ Client closed connection gracefully", BYEL);
		client.state = CLOSING;
		return;
	}

	client.readBuffer.append(buffer, readBytes);
	client.totalReceived += readBytes;

	// Look for the HTTP header terminator: "\r\n\r\n". Until this appears, we only have a partial header block.
	size_t headerEnd = client.readBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		// DoS means Denial of Service. It’s an attack where someone makes a server unavailable by exhausting resources like
		// Anti-DoS guard #1:
		// If a client keeps sending bytes without finishing headers,
		// readBuffer would grow forever. Cap header growth at MAX_HEADER_SIZE.
		if (client.readBuffer.size() > MAX_HEADER_SIZE)
		{
			printLog("🚨 Request headers too large", RED);
			// 431 = header section is too large (more precise than generic 413).
			client.status = 431;
			client.keepAlive = false;
			// Move to write path so we send the error response immediately.
			client.state = WRITING;
		}
		return;
	}

	// headerEnd points to the first '\r' of the terminator, so +4 includes "\r\n\r\n".
	size_t headerSize = headerEnd + 4;
	// Anti-DoS guard #2:
	// Even with a terminator, reject oversized header blocks.
	if (headerSize > MAX_HEADER_SIZE)
	{
		printLog("🚨 Request headers too large", RED);
		client.status = 431;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}

	// Header parsing/inspection happens only after size limits pass.
	// Build a lowercase view of the *header lines only* (exclude request line)
	// so token scans cannot be spoofed via method/path/version text.
	size_t requestLineEnd = client.readBuffer.find("\r\n");
	if (requestLineEnd == std::string::npos || requestLineEnd >= headerEnd)
	{
		printLog("🚨 Malformed request line or headers", RED);
		client.status = 400;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}
	size_t headerStart = requestLineEnd + 2;
	std::string headersLower = toLower(client.readBuffer.substr(headerStart, headerEnd - headerStart));
	if (hasHeaderToken(headersLower, "transfer-encoding:", "chunked"))
	{
		//MISSING CHUNKED PART
		// Chunked request bodies are detected, but actual chunk decoding is not
		// implemented yet, so return 501 Not Implemented.
		client.status = 501;
		client.keepAlive = false;
		client.state = WRITING; //WHEN CHUNKED IS FIXED CHANGE TO PROCESSING
		return;
	}

	if (client.contentLength == 0)
	{
		// Parse Content-Length once from headers and validate numeric format.
		if (!parseContentLengthValue(headersLower, client.contentLength))
		{
			printLog("🚨 Invalid Content-Length", RED);
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}
	}

	// If declared body is larger than configured upload limit, fail early.
	if (client.contentLength > maxUploadSize)
	{
		printLog("🚨 Content-Length exceeds maximum limit", RED);
		client.state = WRITING;
		client.keepAlive = false;
		client.status = 413;
		return;
	}

	// Overflow-safe guard before computing (headerSize + maxUploadSize).
	// Prevents wrapping size_t on pathological configuration/input combinations.
	if (headerSize > std::numeric_limits<size_t>::max() - maxUploadSize)
	{
		printLog("🚨 Request size overflow guard triggered", RED);
		client.keepAlive = false;
		client.status = 413;
		client.state = WRITING;
		return;
	}

	// Total request budget = bounded headers + bounded body.
	// This prevents unbounded growth even after headers are complete.
	size_t maxRequestSize = headerSize + maxUploadSize;
	if (client.readBuffer.size() > maxRequestSize)
	{
		printLog("🚨 Request exceeds configured total size", RED);
		client.keepAlive = false;
		client.status = 413;
		client.state = WRITING;
		return;
	}

	{
		size_t bodySize = client.readBuffer.size() - headerSize;
		// Once enough body bytes have arrived, the request is complete and can be parsed.
		if (bodySize >= client.contentLength)
			client.state = PROCESSING;
	}
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
