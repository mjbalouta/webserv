#pragma once

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../ServerManager/Request.hpp"
#include "../config/parser/ConfigParser.hpp"

class ConfigResolve
{
	private:
	const ServerConfig* _server;
	const LocationConfig* _location;

	const ServerConfig* findServerBlock(const ConfigParser& config, const Request& request);
	const LocationConfig* findLocationBlock(const ConfigParser& config, const Request& request);

	public:
	ConfigResolve(const ConfigParser& config, const Request& request);

	const ServerConfig& getServer() const;
	const LocationConfig& getLocation() const;
};