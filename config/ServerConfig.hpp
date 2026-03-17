#pragma once

#include "Includes.hpp"
#include "LocationConfig.hpp"

class LocationConfig;

class ServerConfig
{
	private:
	int _fd;
	std::vector<int> _ports; //should it be a vector of ports???
	bool _portDefined; //config file defined a port?
	std::string _root; //base directory where files are served from'
	bool _rootDefined;
	std::vector<std::string> _indexes; //the default file served when a directory is requested 
	std::map<int, std::string> _errorPages; //custom HTML pages for HTTP errors
	std::string _host; //the IP address the server binds to
	std::vector<std::string> _serverNames; //the domain names this server responds to (one machine can host multiple websites on the same port)
	std::vector<LocationConfig> _locations; //all location blocks inside this server
	unsigned long _maxBodySize;
	bool _maxBodySizeFlag;
	bool _autoIndex;
	bool _autoIndexSet; //activated if autoindex exists in the config file
	std::vector<std::string> _allowedMethods;

	public:
	ServerConfig();

	void setFd(int fd);
	void addPort(int port);
	void setPortDefined(bool portDefined);
	void setRoot(const std::string& root);
	void addIndex(const std::string& index);
	void addErrorPages(int code, const std::string& file);
	void setHost(const std::string& host);
	void addServerName(const std::string& serverName);
	void addLocation(const LocationConfig& location);
	void setMaxBodySize(unsigned long size);
	void setMaxBodySizeFlag(bool status);
	void setAutoIndex(bool status);
	void setAutoIndexFlag(bool status);
	void setRootFlag(bool status);
	void addAllowedMethod(const std::string& allowedMethod);

	int getFd() const;
	const std::vector<int>& getPorts() const;
	bool getPortDefined() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::string& getHost() const;
	const std::vector<std::string>& getServerNames() const;
	const std::vector<LocationConfig>& getLocations() const;
	unsigned long getMaxBodySize() const;
	bool getAutoIndex() const;
	bool getAliasFlag() const;
	bool getRootFlag() const;
	const std::vector<std::string>& getAllowedMethods() const;

	void clearIndexes();
	void clearServerNames();
};