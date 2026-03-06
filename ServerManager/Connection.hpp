#pragma once

#include "../Includes.hpp"

class ServerManager;

enum Method{
	GET,
	POST,
	DELETE,
	NONE
};

enum State {
	READING,
	WRITING,
	CLOSING,
	POSSESSING,
	IDLE
};

class Connection {
public:
	Connection(int fd);
	~Connection();
	int _fd;
	State _state;
	time_t _lastActive;
	
	// buffers
	std::string _readBuffer;
	std::string _writeBuffer;
	size_t _totalSent;
	size_t _totalReceived;
	
	// HTTP
	Method _method;
	int _statusCode;
	std::string _path;
	size_t _contentLength;
	bool _keepAlive;
	bool _isRedirection;
	bool _headersSend;

	//getters
	int getFd() const { return _fd; }
	State getState() const { return _state; }
	Method getMethod() const { return _method; }
	int getStatusCode() const { return _statusCode; }
	size_t getContentLength() const { return _contentLength; }
	const std::string& getPath() const { return _path; }
	const std::string& getReadBuffer() const { return _readBuffer; }
	const std::string& getWriteBuffer() const { return _writeBuffer; }
	size_t getTotalSent() const { return _totalSent; }
	size_t getTotalReceived() const { return _totalReceived; }
	time_t getLastActive() const { return _lastActive; }
	
	//booleans
	bool isKeepAlive() const { return _keepAlive; }
	bool IsRedir() const { return _isRedirection; }
	bool areHeadersSent() const { return _headersSend; }

	//class functions
	void handleRequest(ServerManager sm);
	void closeConnection();
	void readRequest(ServerManager sm);
};