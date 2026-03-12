#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
: _fd(-1), _portDefined(false), _root("./www"), _rootDefined(false), _host("0.0.0.0")
, _maxBodySize(1048576), _autoIndex(false)
{}

void ServerConfig::addPort(int port)
{
	_ports.push_back(port);
}

void ServerConfig::setPortDefined(bool portDefined)
{
	_portDefined = portDefined;
}

void ServerConfig::setRoot(const std::string& root)
{
	_root = root;
}

void ServerConfig::addIndex(const std::string& index)
{
	_indexes.push_back(index);
}

void ServerConfig::addErrorPages(int code, const std::string& file)
{
	_errorPages[code] = file;
}

void ServerConfig::setHost(const std::string& host)
{
	_host = host;
}

void ServerConfig::addServerName(const std::string& serverName)
{
	_serverNames.push_back(serverName);
}

void ServerConfig::addLocation(const LocationConfig& location)
{
	_locations.push_back(location);
}

const std::vector<int>& ServerConfig::getPorts() const
{
	return _ports;
}

bool ServerConfig::getPortDefined() const
{
	return _portDefined;
}

const std::string& ServerConfig::getRoot() const
{
	return _root;
}

const std::vector<std::string>& ServerConfig::getIndexes() const
{
	return _indexes;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const
{
	return _errorPages;
}

const std::string& ServerConfig::getHost() const
{
	return _host;
}

const std::vector<std::string>& ServerConfig::getServerNames() const
{
	return _serverNames;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const
{
	return _locations;
}

void ServerConfig::clearIndexes()
{
	_indexes.clear();
}

void ServerConfig::setMaxBodySize(unsigned long size)
{
	_maxBodySize = size;
}

unsigned long ServerConfig::getMaxBodySize() const
{
	return _maxBodySize;
}

void ServerConfig::setAutoIndex(bool status)
{
	_autoIndex = status;
}

bool ServerConfig::getAutoIndex() const
{
	return _autoIndex;
}

void ServerConfig::clearServerNames()
{
	_serverNames.clear();
}

bool ServerConfig::getAliasFlag() const
{
	return false;
}

void ServerConfig::setFd(int fd)
{
	_fd = fd;
}

int ServerConfig::getFd() const
{
	return _fd;
}

void ServerConfig::setRootFlag(bool status)
{
	_rootDefined = status;
}

bool ServerConfig::getRootFlag() const
{
	return _rootDefined;
}
