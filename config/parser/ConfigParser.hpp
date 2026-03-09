#pragma once

#include "Includes.hpp"

class ConfigParser
{
	private:
	std::vector<ServerConfig> _servers; //should it be a map container to store the port more directly?
	std::string _filename; //config file path
	std::vector<std::string> _tokens; //to store the lines of the config file
	size_t _currentToken; //to store in which line i'm in

	void tokenize(std::string content);
	void parseServer();
	void parseListen(ServerConfig& server);
	template <typename T>
	void parseRoot(T& object);
	void parseHost(ServerConfig& server);
	void parseServerName(ServerConfig& server);
	void parseIndex(ServerConfig& server);
	void parseErrorPage(ServerConfig& server);
	void parseMaxBodySize(ServerConfig& server);
	void parseAutoindex(ServerConfig& server);
	void parseLocation(ServerConfig& server);
	void parseAlias(LocationConfig& location);
	void checkIfTokenExists();

	public:
	ConfigParser(const std::string& filename);
	const std::vector<ServerConfig>& getServers() const;
	void parse();
};