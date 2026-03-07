#include "ConfigParser.hpp"


void ConfigParser::parseListen(ServerConfig& server)
{
	if (++_currentToken > _tokens.size())
		throw ConfigException("Error: Unexpected end of file.");
	
	//check if the next token is all numbers
	std::string currentToken = _tokens[_currentToken];
	int isPort = 1;
	for (size_t i = 0; i < currentToken.size(); i++)
	{
		if (currentToken[i] < '0' || currentToken[i] > '9')
		{
			isPort = 0;
			break;
		}
	}
	if (isPort)
	{
		int port = atoi(currentToken.c_str());
		if (port < 1 || port > 65535)
			throw ConfigException("Error: Invalid port " + currentToken);
	}
	else
	{
		//verify if it is host only or host and port
		// can be listen 80; listen localhost; listen 127.0.0.1; listen 127.0.0.1:8080
	}

}

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
		//end of the block
		if (_tokens[_currentToken] == "}")
			break;
	
		if (_tokens[_currentToken].find("listen") == 0)
			parseListen(server);
		else if (_tokens[_currentToken].find("root") == 0)
			parseRoot();
		else if (_tokens[_currentToken].find("host") == 0)
			parseHost();
		else if (_tokens[_currentToken].find("server_name") == 0)
			parseServerName();
		else if (_tokens[_currentToken].find("index") == 0)
			parseIndex();
		else if(_tokens[_currentToken].find("error_page") == 0)
			parseErrorPage();
		else if (_tokens[_currentToken].find("location") == 0)
			parseLocation();
		else
			throw ConfigException("Error: Unknown config in server block at line" + _currentLine);	
	}
	_servers.push_back(server);
}