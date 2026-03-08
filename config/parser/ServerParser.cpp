#include "ConfigParser.hpp"

/**
 * @brief Validates information after 'host' keyword
 * 
 * @param server 
 */
void ConfigParser::parseHost(ServerConfig& server)
{
	if (++_currentToken >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);

	std::string host = _tokens[_currentToken];

	if (host == ";")
		throw ConfigException("Error: There must be a valid IP or hostname after keyword 'host");
	
	if (_currentToken + 1 >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);
	if (_tokens[_currentToken + 1] != ";")
		throw ConfigException("Error: Expected a ';' after IP or hostname.");
	
	ConfigUtils::validateHost(host, server);
	_currentToken++;
}

/**
 * @brief Validates information after 'root' keyword
 * 
 */
void ConfigParser::parseRoot(ServerConfig& server)
{
	if (++_currentToken >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);

	std::string rootPath = _tokens[_currentToken];

	if (rootPath == ";")
		throw ConfigException("Error: There must be a valid path after 'root' keyword.");

	//there can only be one token between the 'root' word and the ';', because the path can't have spaces in between
	if (_currentToken + 1 >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);
	if (_tokens[_currentToken + 1] != ";")
		throw ConfigException("Error: Unknown path detected after 'root'.");

	server.setRoot(_tokens[_currentToken]);	
	_currentToken++;
}

/**
 * @brief Validates information after the 'listen' keyword
 * 
 * @param server 
 */
void ConfigParser::parseListen(ServerConfig& server)
{
	if (++_currentToken >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);
	
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
		//check if format indicates an host:port
		size_t pos = currentToken.find(':');
		if (pos != std::string::npos)
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
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);
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
	
		if (_tokens[_currentToken] == "listen")
			parseListen(server);
		else if (_tokens[_currentToken] == "root")
			parseRoot(server);
		else if (_tokens[_currentToken] == "host")
			parseHost(server);
		else if (_tokens[_currentToken] == "server_name")
			parseServerName(server);
		else if (_tokens[_currentToken] == "index")
			parseIndex(server);
		else if(_tokens[_currentToken] == "error_page")
			parseErrorPage(server);
		else if(_tokens[_currentToken] == "client_max_body_size")
			parseMaxBodySize(server);
		else if(_tokens[_currentToken] == "autoindex")
			parseAutoindex(server);
		else if (_tokens[_currentToken] == "location")
			parseLocation(server);
		else
			throw ConfigException("Error: Unknown config " + _tokens[_currentToken]);	
	}
	_servers.push_back(server);
}