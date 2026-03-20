#pragma once

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../ServerManager/Request.hpp"

class ConfigResolved
{
	private:
	const ServerConfig* _server;
	const LocationConfig* _location;

	const LocationConfig* findLocationBlock(const ServerConfig& server, const Request& request);
	
	public:
	ConfigResolved(const Request& request, const ServerConfig& server);
	
	std::vector<int> getPorts() const;
	std::string getLocationPath() const;
	std::string getResolvedPath(const Request& request) const;
	const std::string& getRoot() const;
	std::string getAlias() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getAllowedMethods() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::string& getHost() const;
	const std::vector<std::string>& getServerNames() const;
	unsigned long getMaxBodySize() const;
	bool getAutoIndex() const;
	int getReturnStatusCode() const;
	std::string getReturnURL() const;
	std::string getReturnMessage() const;
	const std::map<std::string, std::string>& getCgi() const;
	std::string getUploadStore() const;
};