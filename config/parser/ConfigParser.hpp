#pragma once

#include "Includes.hpp"

class ConfigParser
{
	private:
	std::vector<ServerConfig> _servers; //should it be a map container to store the port more directly?
	std::vector<std::string> _tokens; //to store the lines of the config file
	size_t _currentToken; //to store in which line i'm in

	void tokenize(std::string& content);
	void parseServer();
	void parseListen(ServerConfig& server);
	void parseHost(ServerConfig& server);
	void parseServerName(ServerConfig& server);
	void parseLocation(ServerConfig& server);
	void parseAlias(LocationConfig& location);
	void parseAllowMethods(LocationConfig& location);
	void parseReturn(LocationConfig& location);
	template <typename T>
	void parseRoot(T& object);
	template <typename T>
	void parseIndex(T& object);
	template <typename T>
	void parseErrorPage(T& object);
	template <typename T>
	void parseMaxBodySize(T& object);
	template <typename T>
	void parseAutoindex(T& object);
	void checkIfTokenExists();

	public:
	ConfigParser();
	const std::vector<ServerConfig>& getServers() const;
	void parse(const std::string& filename);
};