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
	const std::string getLocationPath() const;

	public:
	ConfigResolved(const ConfigParser& config, const Request& request, const ServerConfig& server);

	const ServerConfig& getServer() const;
	const LocationConfig& getLocation() const;

	const std::string getResolvedPath(const Request& request) const;
	const std::string getRoot() const;
	const std::string getAlias() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getAllowedMethods() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::string getHost() const;
	const std::vector<std::string>& getServerNames() const;
	unsigned long getMaxBodySize() const;
	bool getAutoIndex() const;
	int getReturnStatusCode() const;
	const std::string getReturnURL() const;
	const std::string getReturnMessage() const;
	const std::map<std::string, std::string>& getCgi() const;
	const std::string getUploadStore() const;
};