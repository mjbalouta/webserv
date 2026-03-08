#include "ConfigParser.hpp"

/**
 * @brief Validates information after the 'listen' keyword
 * 
 * @param server 
 */
void ConfigParser::parseListen(ServerConfig& server)
{
	if (++_currentToken >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file.");
	
	bool isPort = false;
	//check if the next token contains any character that is not a number: if it has not, it is a port
	std::string currentToken = _tokens[_currentToken];
	if (allDigits(currentToken))
	{
		ConfigUtils::validatePort(currentToken, server);
		isPort = true;
	}
	
	if (!isPort)
	{
		size_t pos = currentToken.find(':');
		if (pos != std::string::npos) //check if format indicates an host:port
		{
			std::string host = currentToken.substr(0, pos);
			std::string port = currentToken.substr(pos + 1);
			if (host.empty() || port.empty())
				throw ConfigException("Error: Invalid host:port format " + currentToken);
			ConfigUtils::validateHost(host, server);
			ConfigUtils::validatePort(port, server);
		}
		else //assume it's host format
			ConfigUtils::validateHost(currentToken, server);	
	}

	if (++_currentToken >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file.");
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected ';' token after " + _tokens[_currentToken - 1]);
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