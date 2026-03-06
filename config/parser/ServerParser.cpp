#include "ConfigParser.hpp"

/**
 * @brief After finding the server block, it creates a ServerConfig
 * object and stores everything inside the block inside this object
 * 
 */
void ConfigParser::parseServer()
{
	//ola
	ServerConfig server;
	int foundBracket = 0;

	//check if there are words between server and {
	size_t serverPos = _lines[_currentLine].find("server");
	size_t afterServerPos = serverPos + std::string("server").length();
	std::string afterServer = _lines[_currentLine].substr(afterServerPos);
	afterServer = trimSpaces(afterServer);
	if (afterServer.find('{') != 0)
		throw ConfigException("Error: Expected '{' after server keyword.");

	//look for the { start bracket in the same line as server
	size_t bracketPos = _lines[_currentLine].find('{');
	if (bracketPos == std::string::npos)
	{
		//look for the { start bracket in a different line
		while (++_currentLine < _lines.size())
		{
			std::string line = _lines[_currentLine];
			line = trimSpaces(line);
			if (line.empty())
				continue;
			if (line == "{")
			{
				foundBracket = 1;
				break;
			}
		}
		if (!foundBracket)
			throw ConfigException("Error: Expected '{' after server keyword.");
	}

	//look for the server block content
	while (++_currentLine < _lines.size())
	{
		std::string line = _lines[_currentLine];
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