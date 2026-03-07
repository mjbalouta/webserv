#pragma once

#include "Includes.hpp"

class ConfigParser
{
	private:
	std::vector<ServerConfig> _servers;
	std::string _filename; //config file path
	std::vector<std::string> _tokens; //to store the lines of the config file
	size_t _currentToken; //to store in which line i'm in

	void parseServer();

	public:
	ConfigParser(const std::string& filename);
	const std::vector<ServerConfig>& getServers() const;
	void parse();
};