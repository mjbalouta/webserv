#pragma once
#include "Includes.hpp"

// Base exception for the project
class AppException : public std::exception {
protected:
	std::string _message;
public:
	AppException(const std::string &msg) : _message(msg) {}
	virtual const char* what() const throw() { return _message.c_str(); }
	virtual ~AppException() throw() {}
};

class RequestException : public AppException {
public:
	RequestException(const std::string &msg) : AppException(msg) {}
};

class SocketException : public AppException {
public:
	SocketException(const std::string &msg) : AppException(msg) {}
};

class FileException : public AppException {
public:
	FileException(const std::string &msg) : AppException(msg) {}
};

class ConfigException : public AppException {
public:
	ConfigException(const std::string &msg) : AppException(msg) {}
};

/* 
exemplo para seguir caso novas exceptions

class ServerException : public AppException {
public:
    ServerException(const std::string &msg) : AppException(msg) {}
};

// USAGE
#include "Exception.hpp"
throw RequestException("Invalid request line");
*/