#pragma once

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../ServerManager/Request.hpp"
#include "../config/parser/ConfigParser.hpp"

class ConfigResolved
{
	private:
	const ServerConfig* _server;
	const LocationConfig* _location;

	const LocationConfig* findLocationBlock(const ServerConfig& server, const Request& request);

	public:
	ConfigResolved(const ConfigParser& config, const Request& request, const ServerConfig& server);

	const ServerConfig& getServer() const;
	const LocationConfig& getLocation() const;

	const std::string getResolvedPath(const Request& request) const;
	const std::string getRoot() const;
};