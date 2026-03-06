#include "Connection.hpp"
#include "../Utils.hpp"

Connection::Connection(int fd) : _fd(fd), _statusCode(200), _isRedirection(false),
								  _readBuffer(""), _writeBuffer(""), _path(""),
								  _method(NONE), _lastActive(time(NULL)),
								  _contentLength(0), _totalSent(0), _totalReceived(0),
								  _keepAlive(true), _headersSend(false), _state(IDLE)
{
}

Connection::~Connection()
{
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
			readRequest(maxUploadSize, epollFd);
			break;
		case PROCESSING:
			break;
		case WRITING:
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
		_statusCode = 413; //payload too large
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