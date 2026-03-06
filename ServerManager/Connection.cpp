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

void Connection::closeConnection()
{
	if (_fd >= 0)
	{
		close(_fd);
		_fd = -1;
	}
	_state = CLOSING;
}

void Connection::handleRequest(ServerManager sm){
	switch (_state)
	{
		case CLOSING:
			printMessage("❌ Connection is closing by state close", RED);
			closeConnection();
			return;
		case READING:
			readRequest(sm);
			break;
		case POSSESSING:
			parseRequest();
			break;
		case WRITING:
			sendResponse();
			break;
		case IDLE:
		default:
			break;
	}
	_lastActive = time(0);
}

/*void Connection::readRequest(ServerManager sm){
	if (_fd < 0){
		printMessage("❌ No socket available", RED);
		_state = CLOSING;
		return;
	}
	
	char buffer[BUFFER_SIZE];
	int readBytes = recv(_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
	if (readBytes < 0){
		printMessage("❌ Failed to read from socket", RED);
		_state = CLOSING;
		return;
	}

	if (readBytes == 0){
		printMessage("❌ Connection closed by client", RED);
		_state = CLOSING;
		return;
	}
	_readBuffer.append(buffer, readBytes);
	_totalReceived += readBytes;
	if (_contentLength == 0){

	}
}


void Run::readRequest(Connection *conn)
{
	if (conn->fd <= 0) {
		print_message("❌ No socket available", RED);
		conn->state = Connection::CLOSING;
		return;
	}
	if (!conn){
		print_message("❌ Connection object is NULL", RED);
		conn->state = Connection::CLOSING;
		return;
	}
	char buffer[BUFFER_SIZE];
	int read_bytes = recv(conn->fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
	if (read_bytes < 0) {
		print_message("❌ Failed to read from socket", RED);
		conn->state = Connection::CLOSING;
		return;
	}

	if (read_bytes == 0) {
		print_message("❌ Connection closed by client", RED);
		conn->state = Connection::CLOSING;
		return;
	}
	conn->read_buffer.append(buffer, read_bytes);
	conn->total_received += read_bytes;
	if (conn->content_length == 0) {
		const std::string content_length_header = "Content-Length: ";
		size_t pos = conn->read_buffer.find(content_length_header);
		if (pos != std::string::npos) {
			size_t start = pos + content_length_header.length();
			size_t end = conn->read_buffer.find("\r\n", start);
			if (end != std::string::npos) {
				try {
					conn->content_length = stolg(conn->read_buffer.substr(start, end - start));
				} catch (const std::exception& e) {
					print_message("❌ Error parsing Content-Length: " + itosg(e.what()), RED);
					close_connection(conn);
					return;
				}
			}
		}
	}
	if ((long)conn->content_length > this->servers[this->currIndexServer]->getuploadSize() || (long)conn->content_length > 5000000000)
	{
		print_message("❌ Content-Length exceeds maximum limit", RED);
		conn->state = Connection::POSSESSING;
		conn->keep_alive = false; 
		conn->status_code = 413;
		mod_epoll(conn->fd, EPOLLOUT);
	}
	else if (conn->content_length > 0) {
		size_t header_end = conn->read_buffer.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			size_t header_size = header_end + 4;
			if (conn->read_buffer.size() >= header_size + conn->content_length) {
				conn->state = Connection::POSSESSING;
				mod_epoll(conn->fd, EPOLLOUT);
			}
		}
	}else if (conn->content_length == 0) {
		conn->state = Connection::POSSESSING;
		mod_epoll(conn->fd, EPOLLOUT);
	}

}

if (_contentLength == 0) {
	const std::string header = "Content-Length: ";
	size_t pos = _readBuffer.find(header);
	
	if (pos != std::string::npos) {
		size_t start = pos + header.length();
		size_t end = _readBuffer.find("\r\n", start);
		
		if (end != std::string::npos) {
			std::string valueStr = _readBuffer.substr(start, end - start);
			
			// Trim whitespace
			size_t first = valueStr.find_first_not_of(" \t");
			size_t last = valueStr.find_last_not_of(" \t");
			
			if (first != std::string::npos) {
				valueStr = valueStr.substr(first, last - first + 1);
				
				// Simple integer conversion without exception
				char *endptr;
				long value = std::strtol(valueStr.c_str(), &endptr, 10);
				
				// Validate: all consumed, positive
				if (*endptr == '\0' && value > 0) {
					_contentLength = (size_t)value;
				} else {
					printMessage("❌ Invalid Content-Length value", RED);
					_state = CLOSING;
					return;
				}
			}
		}
	}
}*/