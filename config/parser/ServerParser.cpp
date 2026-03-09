#include "ConfigParser.hpp"

/**
 * @brief Validates information after 'autoindex' keyword
 * 
 * @param server 
 */
void ConfigParser::parseAutoindex(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'autoindex'.");

	if (_tokens[_currentToken] == "on")
		server.setAutoIndex(true);
	else if (_tokens[_currentToken] == "off")
		server.setAutoIndex(false);
	else
		throw ConfigException("Error: Invalid definition of autoindex: " + _tokens[_currentToken]);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after autoindex definition.");
}

/**
 * @brief Validates information after 'client_max_body_size' keyword
 * 
 * @param server 
 */
void ConfigParser::parseMaxBodySize(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'client_max_body_size'.");

	std::string token = _tokens[_currentToken];
	int optionConversion = 0;
	unsigned long max;
	if (allDigits(token))
		max = atol(token.c_str());
	else
	{
		if (token.size() < 2)
			throw ConfigException("Error: Invalid definition of client_max_body_size.");
		size_t pos = token.find_first_not_of("0123456789");
		if (pos != token.size() - 1)
			throw ConfigException("Error: Wrong definition of client_max_body_size.");
		
		if (token[token.size() - 1] == 'k' || token[token.size() - 1] == 'K')
			optionConversion = 1;
		else if (token[token.size() - 1] == 'm' || token[token.size() - 1] == 'M')
			optionConversion = 2;
		else if (token[token.size() - 1] == 'g' || token[token.size() - 1] == 'G')
			optionConversion = 3;
		else
			throw ConfigException("Error: Conversion for client_max_body_size not possible.");

		std::string numberPart = token.substr(0, token.size() - 1);
		unsigned long size = atol(numberPart.c_str());
		max = ConfigUtils::calculateSize(size, optionConversion);
	}

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after client_max_body_size definitions.");

	server.setMaxBodySize(max);
}

/**
 * @brief Validates information after 'error_page' keyword
 * 
 * @param server 
 */
void ConfigParser::parseErrorPage(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'error_page'.");

	std::vector<int> codes;
	std::string path;
	while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		std::string token = _tokens[_currentToken];
		if (_currentToken + 1 < _tokens.size() && _tokens[_currentToken + 1] == ";")
		{
			ConfigUtils::validateErrorPagePath(token);
			path = token;
		}
		else
		{
			ConfigUtils::validateStatusCode(token);
			int code = atoi(token.c_str());
			codes.push_back(code);
		}
		_currentToken++;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after error_page definitions.");

	if (path.empty())
		throw ConfigException("Error: Missing path definition after error status codes for error_page.");
	if (codes.empty())
		throw ConfigException("Error: Missing definition of status codes for error_page.");

	for (std::vector<int>::iterator it = codes.begin(); it != codes.end(); ++it)
		server.addErrorPages(*it, path);

}

/**
 * @brief Validates information after 'index' keyword
 * 
 * @param server 
 */
void ConfigParser::parseIndex(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'index'.");

	//if there are more than one definition of 'index', we must clear the previous one
	//because the last definition should overwrite any previous one
	server.clearIndexes();

	while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		std::string token = _tokens[_currentToken];
		ConfigUtils::validateFilename(token);
		server.addIndex(token);
		_currentToken++;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after index definitions.");
}

/**
 * @brief Validates information after 'server_name' keyword
 * 
 * @param server 
 */
void ConfigParser::parseServerName(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definitions after 'servername' keyword.");

	std::string errorMessage = "Error: Invalid server_name: " + _tokens[_currentToken];
	while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		std::string token = _tokens[_currentToken];
		//server_name follows the same format rules as hostname
		ConfigUtils::validateHostname(token, errorMessage);
		server.addServerName(token);
		_currentToken++;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after server_name definitions.");
}

/**
 * @brief Validates information after 'host' keyword
 * 
 * @param server 
 */
void ConfigParser::parseHost(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	std::string host = _tokens[_currentToken];

	if (host == ";")
		throw ConfigException("Error: There must be a valid IP or hostname after keyword 'host");
	
	ConfigUtils::validateHost(host, server);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after IP or hostname.");

}

/**
 * @brief Validates information after the 'listen' keyword
 * 
 * @param server 
 */
void ConfigParser::parseListen(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();
	
	bool isPort = false;
	//check if the next token contains any character that is not a number: if it has not, it is a port
	std::string currentToken = _tokens[_currentToken];
	if (allDigits(currentToken))
	{
		ConfigUtils::validatePort(currentToken, server);
		server.setPortDefined(true);
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

	++_currentToken;
	checkIfTokenExists();
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

	int endBracket = 0;
	//look for the server block content
	while (++_currentToken < _tokens.size())
	{
		//end of the block
		if (_tokens[_currentToken] == "}")
		{
			endBracket = 1;
			break;
		}
		else if (_tokens[_currentToken] == "listen")
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
			throw ConfigException("Error: Unknown keyword " + _tokens[_currentToken]);	
	}
	if (!endBracket)
		throw ConfigException("Error: Expected '}' in the end of location block.");
	_servers.push_back(server);
}