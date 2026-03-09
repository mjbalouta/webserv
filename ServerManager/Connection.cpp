#include "Connection.hpp"

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
void Connection::handleRequest(size_t maxUploadSize, int epollFd, Config &config){
	switch (_state)
	{
		case CLOSING:
			printMessage("❌ Connection is closing by state close", RED);
			closeConnection();
			return;
		case READING:
			readRequest(maxUploadSize, epollFd);
			break;
		case PROCESSING:
			parseRequest(config);
			break;
		case WRITING:
			//sendRequest();
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
void Connection::readRequest(size_t maxUploadSize, int epollFd){

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
		printMessage("❌ Failed to read from socket", RED);
		_state = CLOSING;
		return;
	}

	// 3b) Peer closed connection.
	if (readBytes == 0){
		printMessage("❌ Connection closed by client", RED);
		_state = CLOSING;
		return;
	}

	// 3c) Successful read: append chunk and account total bytes.
	_readBuffer.append(buffer, readBytes);
	_totalReceived += readBytes;

	// 4) Parse Content-Length once (if not parsed yet).
	if (_contentLength == 0){
		const std::string header = "Content-Length:";
		size_t pos = _readBuffer.find(header);

		if (pos != std::string::npos) {
			// Move to header value start, skipping optional spaces/tabs.
			size_t start = pos + header.length();
			while (start < _readBuffer.size() && (_readBuffer[start] == ' ' || _readBuffer[start] == '\t'))
				++start;

			// Header value ends at CRLF.
			size_t end = _readBuffer.find("\r\n", start);

			if (end != std::string::npos) {
				std::string valueStr = _readBuffer.substr(start, end - start);

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
		modEpoll(epollFd, _fd, EPOLLOUT);
	}
	else if (_contentLength > 0) {
		size_t headerEnd = _readBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			size_t headerSize = headerEnd + 4;
			if (_readBuffer.size() >= headerSize + _contentLength) //full request body was read
			{
				_state = PROCESSING;
				modEpoll(epollFd, _fd, EPOLLOUT);
			}
		}
	}else if (_contentLength == 0) {
		_state = PROCESSING;
		modEpoll(epollFd, _fd, EPOLLOUT);
	}
}

/**
 * @brief Parses HTTP request from connection buffers and routes to appropriate handler.
 * @param config Server configuration for routing and error page resolution.
 * @return Request object containing parsed request metadata.
 */
Request Connection::parseRequest(Config &config) {
	Request request;
	// Parse incoming raw request stored in connection buffers.
	if (!request.parseRequest(config)){
		// Parsing failed: clear pending output and preserve parser status.
		_writeBuffer.clear();
		_path = "";
		_status = request.getStatus();
	} else {
		// Redirect response is ready immediately.
		if (request.isRedirect()) {
			_isRedirection = true;
			_status = request.getStatus();
			preparePageFile(config); //sei laaa
			_state = WRITING;
		}
		// DELETE is delegated to dedicated handling path.
		if (request.getMethod() == DELETE){
			_method = DELETE;
			processRequest(config, request);
			return request;
		}
		// Autoindex already provides response body content.
		if (request.isAutoIndex()){
			_status = request.getStatus();
			_responseStr = request.getAutoIndexPath();
			if (_responseStr.empty()){
				_status = 400;
				printMessage("Error: Autoindex path is empty", RED);
				preparePageFile(config);
				_state = WRITING;
				return request;
			}
			_state = WRITING;
			return request;
		}
	}
	// Request is consumed; next stage uses resolved method/path/status.
	_readBuffer.clear();
	if (_method == NONE && _status == 200)
		_method = request.getMethod();
	processRequest(config, request);
	return request;
}

/**
 * @brief Resolves request target path based on method and schedules response.
 * @param config Active server configuration used for page/error resolution.
 * @param request Parsed HTTP request metadata.
 */
void Connection::processRequest(Config &config, Request &request){
	// GET/POST serve request path directly.
	if (_method == GET || _method == POST)
		_path = request.getPath();
	else if (_method == DELETE)
		deleteHandle(request); //TODO
	else {
		_status = 400;
		printMessage("Unknown request", RED);
	}
	// Open final resource (or fallback error page) and move to write phase.
	preparePageFile(config);
	_state = WRITING;
};

/**
 * @brief Selects a file path and opens it for response sending.
 * @details
 *   1) Try configured error page for current status code.
 *   2) Fallback to default error page when missing.
 *   3) Open file stream and cache file size in `_contentLength`.
 * @param config Active server configuration.
 */
void Connection::preparePageFile(const Config &config)
{
	// 1) Map HTTP status code to configured error page path.
	std::map<int, std::string> errorPages = config.getErrorPage();
	int code = _status;
	for (std::map<int, std::string>::iterator it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		if (it->first == code)
		{
			_path = it->second;
			break;
		}
	}
	// 2) If no specific page exists, use default error page.
	if (_path.empty())
		_path = "www/error_pages/default_error.html";

	// 3) Open selected file in binary mode.
	// Allocate stream if not already created.
	if (!_readFromFile)
		_readFromFile = new std::ifstream();
	
	// Close existing stream if open.
	if (_readFromFile->is_open())
		_readFromFile->close();
	
	_readFromFile->open(_path.c_str(), std::ios::in | std::ios::binary);
	if (!_readFromFile->is_open())
	{
		printMessage("Failed to open error file", RED);
		_path = "www/error_pages/default_error.html";
		// Retry once with guaranteed fallback path.
		_readFromFile->open(_path.c_str(), std::ios::in | std::ios::binary);
		if (!_readFromFile->is_open())
		{
			printMessage("Failed to open default error file", RED);
			return;
		}
	}
	// 4) Cache content size for Content-Length header.
	struct stat st;
	stat(_path.c_str(), &st);
	_contentLength = st.st_size;
	return;
}