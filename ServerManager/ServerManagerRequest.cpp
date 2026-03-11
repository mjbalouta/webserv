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
static std::string buildDefaultResponse(int statusCode, bool keepAlive, const std::string &explicitBody)
{
	//Person 3 ownership: replace this fallback with the final Response engine serializer.
	std::string body = explicitBody;
	if (body.empty())
		body = getReasonPhrase(statusCode) + "\n";

	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " " << getReasonPhrase(statusCode) << "\r\n";
	oss << "Content-Type: text/plain\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
	oss << "\r\n";
	oss << body;
	return oss.str();
}

/**
 * @brief Checks if a given header line contains a specific token.
 */
static bool hasHeaderToken(const std::string &headersLower, const std::string &headerName, const std::string &token)
{
	size_t headerPos = headersLower.find(headerName);
	if (headerPos == std::string::npos)
		return false;

	size_t lineEnd = headersLower.find("\r\n", headerPos);
	if (lineEnd == std::string::npos)
		lineEnd = headersLower.size();

	return headersLower.substr(headerPos, lineEnd - headerPos).find(token) != std::string::npos;
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
	size_t pos = headersLower.find(header);
	if (pos == std::string::npos)
		return true;

	size_t start = pos + header.length();
	while (start < headersLower.size() && (headersLower[start] == ' ' || headersLower[start] == '\t'))
		++start;

	size_t end = headersLower.find("\r\n", start);
	if (end == std::string::npos)
		return true;

	std::string valueStr = headersLower.substr(start, end - start);
	char *endptr = NULL;
	long value = std::strtol(valueStr.c_str(), &endptr, 10);
	if (endptr == valueStr.c_str() || *endptr != '\0' || value < 0)
		return false;

	outContentLength = static_cast<size_t>(value);
	return true;
}

/**
 * @brief Advances one client state-machine step based on current state.
 * @param client Client session.
 * @param maxUploadSize Server upload limit.
 * @param epollFd Epoll instance descriptor (kept for interface consistency).
 */
void ServerManager::handleClientRequest(ClientSession &client, size_t maxUploadSize, int epollFd)
{
	(void)epollFd;
	switch (client.state)
	{
		case CLOSING:
			printLog("ℹ️ Client already marked closing", BYEL);
			closeClientSocket(client);
			return;
		case READING:
			readClientRequest(client, maxUploadSize);
			break;
		case PROCESSING:
			parseClientRequest(client);
			if (client.state == WRITING)
				modClientEpoll(client, EPOLLOUT);
			break;
		case WRITING:
			sendClientResponse(client, epollFd);
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
		printLog("🟥 No socket available", RED);
		client.state = CLOSING;
		return;
	}

	char buffer[BUFFER_SIZE];
	int readBytes = recv(client.fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
	// recv() reads up to sizeof(buffer) bytes from socket _fd into buffer.
	// Returns: >0 number of bytes read, 0 if client closed connection, -1 on error (check errno).
	// MSG_NOSIGNAL prevents SIGPIPE-related signals during socket operations.
	if (readBytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return;
		if (errno == ECONNRESET)
			printLog("🟥 Client connection reset during recv", RED);
		else
			printLog("🟥 Failed to read from socket", RED);
		client.state = CLOSING;
		return;
	}

	if (readBytes == 0)
	{
		printLog("ℹ️ Client closed connection gracefully", BYEL);
		client.state = CLOSING;
		return;
	}

	client.readBuffer.append(buffer, readBytes);
	client.totalReceived += readBytes;

	size_t headerEnd = client.readBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;

	std::string headersLower = toLower(client.readBuffer.substr(0, headerEnd));
	if (hasHeaderToken(headersLower, "transfer-encoding:", "chunked"))
	{
		//MISSING CHUNKED PART
		client.status = 501;
		client.keepAlive = false;
		client.state = PROCESSING;
		return;
	}

	if (client.contentLength == 0)
	{
		if (!parseContentLengthValue(headersLower, client.contentLength))
		{
			printLog("🟥 Invalid Content-Length", RED);
			client.status = 400;
			client.keepAlive = false;
			client.state = PROCESSING;
			return;
		}
	}

	if ((long)client.contentLength > static_cast<long>(maxUploadSize))
	{
		printLog("🟥 Content-Length exceeds maximum limit", RED);
		client.state = PROCESSING;
		client.keepAlive = false;
		client.status = 413;
	}
	else
	{
		size_t headerSize = headerEnd + 4;
		size_t bodySize = client.readBuffer.size() - headerSize;
		if (bodySize >= client.contentLength)
			client.state = PROCESSING;
	}
}

/**
 * @brief Parses buffered request text and updates session response metadata.
 * @param client Client session.
 * @return Parsed request object.
 */
Request ServerManager::parseClientRequest(ClientSession &client)
{
	Request request;
	if (!request.parseRequest(client.readBuffer, client.contentLength))
	{
		client.writeBuffer.clear();
		client.path = "";
		client.method = NONE;
		client.status = request.getStatus();
		printLog("⚠️ Malformed HTTP request handled", YEL);
		client.state = WRITING;
	}
	else
		// Person 2 hook: receive parsed Request and decide routing/config result
		// (server/location match, method validation, effective path, status).
		processClientRequest(client, request);

	client.readBuffer.clear();
	client.contentLength = 0;
	return request;
}

/**
 * @brief Copies parsed request metadata into session transport/response fields.
 * @param client Client session.
 * @param request Parsed request object.
 */
void ServerManager::processClientRequest(ClientSession &client, Request &request)
{
	// Person 2 hook: this is where routing/config engine output should be applied
	// to client.status, client.path, redirection flags, and method policy.
	client.method = request.getMethod();
	client.path = request.getPath();
	client.status = request.getStatus();
	client.isRedirection = request.isRedirect();

	std::string clientHeader = toLower(request.getHeader("connection"));
	if (request.getVersion() == "HTTP/1.0")
		client.keepAlive = (clientHeader == "keep-alive");
	else
		client.keepAlive = (clientHeader != "close");

	if (request.isAutoIndex())
		// Person 3 hook: replace this placeholder body source with final
		// resource engine output (file content/error page/rendered directory).
		client.responseStr = request.getAutoIndexPath();
	else
		client.responseStr.clear();
	client.state = WRITING;
}

/**
 * @brief Sends response bytes and handles keep-alive reset/close decisions.
 * @param client Client session.
 * @param epollFd Epoll instance descriptor (kept for interface consistency).
 */
void ServerManager::sendClientResponse(ClientSession &client, int epollFd)
{
	(void)epollFd;
	if (client.fd < 0)
	{
		client.state = CLOSING;
		return;
	}

	if (client.writeBuffer.empty())
		// Person 3 hook: build Response object + headers/body here, then
		// serialize to HTTP text (status line, Content-Length, Connection).
		client.writeBuffer = buildDefaultResponse(client.status, client.keepAlive, client.responseStr);

	ssize_t sentBytes = send(client.fd, client.writeBuffer.c_str() + client.totalSent,
		client.writeBuffer.size() - client.totalSent, MSG_NOSIGNAL);
	if (sentBytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return;
		if (errno == EPIPE)
			printLog("🟥 Broken pipe while sending response", RED);
		else if (errno == ECONNRESET)
			printLog("🟥 Client reset connection while sending response", RED);
		else
			printLog("🟥 Failed to send response", RED);
		client.state = CLOSING;
		return;
	}

	client.totalSent += static_cast<size_t>(sentBytes);
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
		client.readBuffer.clear();
		modClientEpoll(client, EPOLLIN);
	}
	else
		client.state = CLOSING;
}
