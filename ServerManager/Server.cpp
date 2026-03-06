#include "Server.hpp"

Server::Server() : _serverFd(-1), _port(0), _index(0) {}

Server::~Server() {}

// Setters
void Server::setServerFd(int fd) {
	_serverFd = fd;
}

void Server::setPort(int port) {
	_port = port;
}

void Server::setIp(const std::string& ip) {
	_serverIp = ip;
}

void Server::setName(const std::string& name) {
	_serverName = name;
}

void Server::setMaxBody(const std::string& maxBody) {
	_clientMaxBodySize = maxBody;
}

void Server::setRoot(const std::string& root) {
	_defaultRoot = root;
}

void Server::setIndex(int index) {
	_index = index;
}

// Getters
int Server::getServerfd() const {
	return _serverFd;
}

int Server::getPort() const {
	return _port;
}

std::string Server::getServerIp() const {
	return _serverIp;
}

std::string Server::getServerName() const {
	return _serverName;
}
