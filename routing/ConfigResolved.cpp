#include "ConfigResolved.hpp"

ConfigResolve::ConfigResolve(const ConfigParser& config, const Request& request)
{
	_server = findServerBlock(config, request);
	_location = findLocationBlock(config, request);
}

const ServerConfig* ConfigResolve::findServerBlock(const ConfigParser& config, const Request& request)
{
	std::string host = request.getHost();
	std::vector<ServerConfig> serverBlocks = config.getServers();

	for (std::vector<ServerConfig>::iterator serverIt = serverBlocks.begin(); serverIt != serverBlocks.end(); ++serverIt)
	{
		std::vector<std::string> serverNames = serverIt->getServerNames();
		for (std::vector<std::string>::const_iterator namesIt = serverNames.begin(); namesIt != serverNames.end(); ++namesIt)
		{
			if (*namesIt == host)
				return &(*serverIt);
		}
	}
	return (&serverBlocks[0]); //return the first block as a default (even if it isn't a match, the server still needs to respond)
}

const LocationConfig* ConfigResolve::findLocationBlock(const ConfigParser& config, const Request& request)
{
	std::string path = request.getPath();
	std::vector<ServerConfig> serverBlocks = config.getServers();

	for (std::vector<ServerConfig>::iterator serverIt = serverBlocks.begin(); serverIt != serverBlocks.end(); ++serverIt)
	{
		std::vector<LocationConfig> locationBlocks = serverIt->getLocations();
		for (std::vector<LocationConfig>::iterator locationIt = locationBlocks.begin(); locationIt != locationBlocks.end(); ++locationIt)
		{
			std::string locationPath = locationIt->getPath();
			if (locationPath == path)
				return &(*locationIt);
		}
	}
	//what should i return for default?
}