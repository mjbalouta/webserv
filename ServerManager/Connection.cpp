#include "Connection.hpp"

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

static std::string buildDefaultResponse(int statusCode, bool keepAlive, const std::string &explicitBody)
{
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

Connection::Connection(int fd) : _fd(fd), _status(200), _isRedirection(false),
								  _readBuffer(""), _writeBuffer(""), _path(""),
								  _method(NONE), _lastActive(time(NULL)),
								  _contentLength(0), _totalSent(0), _totalReceived(0),
								  _keepAlive(true), _headersSend(false), _state(IDLE),
								  _readFromFile(NULL), _responseStr("")
{
	_readFromFile = new std::ifstream(); 
}

Connection::Connection(const Connection &other)
	: _fd(other._fd),
	  _state(other._state),
	  _lastActive(other._lastActive),
	  _responseStr(other._responseStr),
	  _readBuffer(other._readBuffer),
	  _writeBuffer(other._writeBuffer),
	  _totalSent(other._totalSent),
	  _totalReceived(other._totalReceived),
	  _readFromFile(NULL),
	  _method(other._method),
	  _status(other._status),
	  _path(other._path),
	  _contentLength(other._contentLength),
	  _keepAlive(other._keepAlive),
	  _isRedirection(other._isRedirection),
	  _headersSend(other._headersSend) {}

Connection& Connection::operator=(const Connection &other)
{
	if (this != &other)
	{
		// Close and delete existing file stream if any.
		if (_readFromFile)
		{
			if (_readFromFile->is_open())
				_readFromFile->close();
			delete _readFromFile;
			_readFromFile = NULL;
		}
		_fd = other._fd;
		_state = other._state;
		_lastActive = other._lastActive;
		_responseStr = other._responseStr;
		_readBuffer = other._readBuffer;
		_writeBuffer = other._writeBuffer;
		_totalSent = other._totalSent;
		_totalReceived = other._totalReceived;
		_method = other._method;
		_status = other._status;
		_path = other._path;
		_contentLength = other._contentLength;
		_keepAlive = other._keepAlive;
		_isRedirection = other._isRedirection;
		_headersSend = other._headersSend;
	}
	return *this;
}

Connection::~Connection()
{
	if (_readFromFile)
	{
		if (_readFromFile->is_open())
			_readFromFile->close();
		delete _readFromFile;
		_readFromFile = NULL;
	}
}

/**
 * @brief Closes the client socket and marks the connection as closing.
 */
void Connection::closeConnection()
{
	if (_fd >= 0)
	{
		close(_fd);
		_fd = -1;
	}
	_state = CLOSING;
}

/**
 * @brief Dispatches one processing step according to current connection state.
 * @param maxUploadSize Maximum allowed request body size for this connection.
 */
void Connection::handleRequest(size_t maxUploadSize, int epollFd){
	switch (_state)
	{
		case CLOSING:
			printMessage("❌ Connection is closing by state close", RED);
			closeConnection();
			return;
		case READING:
			readRequest(maxUploadSize);
			break;
		case PROCESSING:
			parseRequest();
			if (_state == WRITING)
				modEpoll(epollFd, _fd, EPOLLOUT);
			break;
		case WRITING:
			sendResponse(epollFd);
			break;
		case IDLE:
		default:
			break;
	}
	_lastActive = time(0);
}


/**
 * @brief Reads incoming HTTP request bytes from the client socket.
 * @param maxUploadSize Maximum allowed request body size for this connection.
 *
 * Flow:    
 * 1) Validate that the socket fd is still valid.
 * 2) Read bytes from the socket using recv().
 * 3) Handle read outcomes:
 *    - < 0: read error (connection moves to CLOSING)
 *    - = 0: client disconnected (connection moves to CLOSING)
 *    - > 0: append bytes to _readBuffer and update _totalReceived
 * 4) If Content-Length is not known yet, try to parse it from headers.
 */
void Connection::readRequest(size_t maxUploadSize){

	// 1) Guard against invalid socket descriptor.
	if (_fd < 0){
		printMessage("❌ No socket available", RED);
		_state = CLOSING;
		return;
	}
	
	// 2) Read raw bytes from the socket.
	char buffer[BUFFER_SIZE];
	int readBytes = recv(_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
	// recv() reads up to sizeof(buffer) bytes from socket _fd into buffer.
	// Returns: >0 number of bytes read, 0 if client closed connection, -1 on error (check errno).
	// MSG_NOSIGNAL prevents SIGPIPE-related signals during socket operations.

	// 3a) Read failure.
	if (readBytes < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return;
		if (errno == ECONNRESET)
			printMessage("❌ Client connection reset during recv", RED);
		else
			printMessage("❌ Failed to read from socket", RED);
		_state = CLOSING;
		return;
	}

	// 3b) Peer closed connection.
	if (readBytes == 0){
		printMessage("ℹ️ Client closed connection gracefully", BYEL);
		_state = CLOSING;
		return;
	}

	// 3c) Successful read: append chunk and account total bytes.
	_readBuffer.append(buffer, readBytes);
	_totalReceived += readBytes;

	// We can only reason about body completeness after full headers are present.
	size_t headerEnd = _readBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;

	std::string headersLower = toLower(_readBuffer.substr(0, headerEnd));
	size_t transferEncodingPos = headersLower.find("transfer-encoding:");
	if (transferEncodingPos != std::string::npos)
	{
		size_t transferLineEnd = headersLower.find("\r\n", transferEncodingPos);
		if (transferLineEnd == std::string::npos)
			transferLineEnd = headersLower.size();
		std::string transferValue = headersLower.substr(transferEncodingPos, transferLineEnd - transferEncodingPos);
		if (transferValue.find("chunked") != std::string::npos)
		{
			size_t chunkedTerminator = _readBuffer.find("\r\n0\r\n\r\n", headerEnd + 4);
			if (chunkedTerminator == std::string::npos)
				return;
			_status = 501;
			_keepAlive = false;
			_state = PROCESSING;
			return;
		}
	}

	// 4) Parse Content-Length once (if not parsed yet).
	if (_contentLength == 0){
		const std::string header = "content-length:";
		size_t pos = headersLower.find(header);

		if (pos != std::string::npos) {
			// Move to header value start, skipping optional spaces/tabs.
			size_t start = pos + header.length();
			while (start < headersLower.size() && (headersLower[start] == ' ' || headersLower[start] == '\t'))
				++start;

			// Header value ends at CRLF.
			size_t end = headersLower.find("\r\n", start);

			if (end != std::string::npos) {
				std::string valueStr = headersLower.substr(start, end - start);

				// Convert to integer and validate full consumption.
				char *endptr = NULL;
				long value = std::strtol(valueStr.c_str(), &endptr, 10);
				if (endptr == valueStr.c_str() || *endptr != '\0' || value < 0) {
					printMessage("❌ Invalid Content-Length", RED);
					_state = CLOSING;
					return;
				}

				// Valid Content-Length parsed successfully.
				_contentLength = static_cast<size_t>(value);
			}
		}
	}
	if ((long)_contentLength > static_cast<long>(maxUploadSize))
	{
		printMessage("❌ Content-Length exceeds maximum limit", RED);
		_state = PROCESSING;
		_keepAlive = false; //force close after response
		_status = 413; //payload too large
	}
	else {
		size_t headerSize = headerEnd + 4;
		size_t bodySize = _readBuffer.size() - headerSize;
		if (bodySize >= _contentLength)
			_state = PROCESSING;
	}
}

/**
 * @brief Parses HTTP request bytes from the connection read buffer.
 * @param config Unused in transport-only parsing stage (kept for interface compatibility).
 * @return Request object containing parsed request metadata.
 */
Request Connection::parseRequest() {
	Request request;
	// Parse incoming raw request stored in connection buffers.
	if (!request.parseRequest(_readBuffer, _contentLength)){
		// Parsing failed: clear pending output and preserve parser status.
		_writeBuffer.clear();
		_path = "";
		_method = NONE;
		_status = request.getStatus();
		printMessage("⚠️ Malformed HTTP request handled", YEL);
		_state = WRITING;
	} else {
		processRequest(request);
	}
	// Request is consumed; next stage uses resolved method/path/status.
	_readBuffer.clear();
	_contentLength = 0;
	return request;
}

/**
 * @brief Prepares parsed request data for downstream routing/response engines.
 * @param config Unused in transport-only processing stage (kept for interface compatibility).
 * @param request Parsed HTTP request metadata.
 */
void Connection::processRequest(Request &request){
	// TODO(Person 2): route Request -> RouteResult (server/location/method/path decision).
	// TODO(Person 3): RouteResult -> final Response payload/status/headers.
	// Person 1 keeps only socket state, buffering, and send/close lifecycle.
	_method = request.getMethod();
	_path = request.getPath();
	_status = request.getStatus();
	_isRedirection = request.isRedirect();

	std::string connectionHeader = toLower(request.getHeader("connection"));
	if (request.getVersion() == "HTTP/1.0")
		_keepAlive = (connectionHeader == "keep-alive");
	else
		_keepAlive = (connectionHeader != "close");

	if (request.isAutoIndex())
		_responseStr = request.getAutoIndexPath();
	else
		_responseStr.clear();
	_state = WRITING;
};

void Connection::sendResponse(int epollFd)
{
	if (_fd < 0)
	{
		_state = CLOSING;
		return;
	}

	if (_writeBuffer.empty())
		_writeBuffer = buildDefaultResponse(_status, _keepAlive, _responseStr);

	ssize_t sentBytes = send(_fd, _writeBuffer.c_str() + _totalSent,
		_writeBuffer.size() - _totalSent, MSG_NOSIGNAL);
	if (sentBytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return;
		if (errno == EPIPE)
			printMessage("❌ Broken pipe while sending response", RED);
		else if (errno == ECONNRESET)
			printMessage("❌ Client reset connection while sending response", RED);
		else
			printMessage("❌ Failed to send response", RED);
		_state = CLOSING;
		return;
	}

	_totalSent += static_cast<size_t>(sentBytes);
	if (_totalSent < _writeBuffer.size())
		return;

	_writeBuffer.clear();
	_totalSent = 0;
	_responseStr.clear();
	_headersSend = true;

	if (_keepAlive)
	{
		_state = READING;
		_method = NONE;
		_status = 200;
		_contentLength = 0;
		_readBuffer.clear();
		modEpoll(epollFd, _fd, EPOLLIN);
	}
	else
		_state = CLOSING;
}