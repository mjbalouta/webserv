#pragma once

#include "../Includes.hpp"

class Config {
	private:
		std::vector<std::string> _port;
		std::vector<std::string> _host;
		std::vector<std::string> _serverName;
		std::vector<std::string> _clientMaxBodySize;
		std::vector<std::string> _defaultRoot;
		std::vector<Config> _configs;
		std::map<int, std::string> _errorPage;
	public:
		Config();
		~Config();
		
		std::vector<std::string> getPort() const;
		std::vector<std::string> getHost() const;
		std::vector<std::string> getServerName() const;
		std::vector<std::string> getClientMaxBodySize() const;
		std::vector<std::string> getDefaultRoot() const;
		std::vector<Config> getConfig() const;
		std::map<int, std::string> getErrorPage() const { return _errorPage; };
};
