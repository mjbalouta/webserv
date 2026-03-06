#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
: _port(80), _portDefined(false), _root("."), _host("0.0.0.0")
{}

void ServerConfig::setPort(int port)
{
	_port = port;
}

void ServerConfig::setPortDefined(bool portDefined)
{
	_portDefined = portDefined;
}

void ServerConfig::setRoot(const std::string& root)
{
	_root = root;
}

void ServerConfig::setIndexes(const std::vector<std::string>& indexes)
{
	_indexes = indexes;
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

int ServerConfig::getPort() const
{
	return _port;
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
