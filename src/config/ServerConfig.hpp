#pragma once

#include "Includes.hpp"
#include "LocationConfig.hpp"

class ServerConfig
{
	private:
	int _port;
	bool _portDefined; //config file defined a port?
	std::string _root; //base directory where files are served from
	std::vector<std::string> _indexes; //the default file served when a directory is requested 
	std::map<int, std::string> _errorPages; //custom HTML pages for HTTP errors
	std::string _host; //the IP address the server binds to
	std::vector<std::string> _serverNames; //the domain names this server responds to (one machine can host multiple websites on the same port)
	std::vector<LocationConfig> _locations; //all location blocks inside this server

	public:
	//MISSING DEFAULT CONSTRUCTORS

	void setPort(int port);
	void setPortDefined(bool portDefined);
	void setRoot(const std::string& root);
	void setIndexes(const std::vector<std::string>& indexes);
	void addErrorPages(int code, const std::string& file);
	void setHost(const std::string& host);
	void addServerName(const std::string& serverName);
	void addLocation(const LocationConfig& location);

	int getPort() const;
	bool getPortDefined() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::string& getHost() const;
	const std::vector<std::string>& getServerNames() const;
	const std::vector<LocationConfig>& getLocations() const;
};