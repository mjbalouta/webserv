#include "ConfigParser.hpp"

/**
 * @brief After finding the server block, it creates a ServerConfig
 * object and stores everything inside the block inside this object
 * 
 */
void ConfigParser::parseServer()
{
	ServerConfig server;
	
	if (_tokens[_currentToken] != "{")
		throw ConfigException("Error: Expected '{' after server keyword.");

	//look for the server block content
	while (++_currentToken < _tokens.size())
	{
		std::string line = _tokens[_currentToken];
		line = trimSpaces(line);

		if (line.empty())
			continue;

		//end of the block
		if (line == "}")
			break;
	
		if (line.find("listen") == 0)
			parseListen();
		else if (line.find("root") == 0)
			parseRoot();
		else if (line.find("host") == 0)
			parseHost();
		else if (line.find("server_name") == 0)
			parseServerName();
		else if (line.find("index") == 0)
			parseIndex();
		else if(line.find("error_page") == 0)
			parseErrorPage();
		else if (line.find("location") == 0)
			parseLocation();
		else
			throw ConfigException("Error: Unknown config in server block at line" + _currentLine);	
	}
	_servers.push_back(server);
}