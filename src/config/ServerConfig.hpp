#pragma once

#include "Includes.hpp"
#include "LocationConfig.hpp"

class ServerConfig
{
	private:
	int _port;
	bool _portDefined; //config file defined a port?
	std::string _root; //base directory where files are served from
	std::string _index; //the default file served when a directory is requested 
	std::map<int, std::string> _errorPages; //custom HTML pages for HTTP errors
	std::string _host; //the IP address the server binds to
	std::vector<std::string> _serverNames; //the domain names this server responds to (one machine can host multiple websites on the same port)
	std::vector<LocationConfig> _locations; //all location blocks inside this server
};