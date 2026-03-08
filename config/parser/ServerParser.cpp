#include "ConfigParser.hpp"

/**
 * @brief Validations for host
 * 
 * @param token 
 * @param server 
 */
void validateHost(std::string& token, ServerConfig& server)
{
	int isIP = 0;
	int isHostname = 0;
	std::string errorMessage = "Error: Invalid host: " + token;

	//check if it has IP format
	size_t pos = token.find_first_not_of(".0123456789");
	if (pos == std::string::npos)
		isIP = 1;
	else
		isHostname = 1;

	if (isIP)
	{
		std::vector<std::string> parts;
		std::stringstream ss(token);
		std::string numbers;
		//if it has IP format, we split by the '.'
		while (std::getline(ss, numbers, '.'))
			parts.push_back(numbers);

		//IP has to have 4 groups of numbers (127.0.0.1)
		if (parts.size() != 4)
			throw ConfigException(errorMessage);
			
		for (size_t i = 0; i < 4; i++)
		{
			//checks if two dots were next to each other + checks it there is any not digit character
			if (parts[i].empty() || !allDigits(parts[i]))
				throw ConfigException(errorMessage);

			int value = std::atoi(parts[i].c_str());
			if (value < 0 || value > 255)
				throw ConfigException(errorMessage);

			//0 is valid, but 01 is invalid
			if (parts[i].size() > 1 && parts[i][0] == '0')
				throw ConfigException(errorMessage);
		}
	}
	if (isHostname)
	{
		if (token[0] == '-' || token[token.size() - 1] == '-')
			throw ConfigException(errorMessage);
		if (token[0] == '.' || token[token.size() - 1] == '.')
			throw ConfigException(errorMessage);
		
		for (size_t i = 0; i < token.size(); i++)
		{
			char c = token[i];
			if (!std::isalnum(c) && c != '.' && c != '-')
				throw ConfigException(errorMessage);
			
			if (i > 0)
			{
				//consecutive ".." / ".-" / "-." are invalid
				if ((c == '.' && token[i - 1] == '.') || (c == '-' && token[i - 1] == '.')
				|| (c == '.' && token[i - 1] == '-'))
					throw ConfigException(errorMessage);
			}
		}
	}
	server.setHost(token);
}

/**
 * @brief Validations for port
 * 
 * @param token 
 * @param server 
 */
void validatePort(std::string& token, ServerConfig& server)
{
	int port = atoi(token.c_str());
	if (port < 1 || port > 65535)
		throw ConfigException("Error: Invalid port " + token);
	server.setPort(port); 
}

void ConfigParser::parseListen(ServerConfig& server)
{
	if (++_currentToken > _tokens.size())
		throw ConfigException("Error: Unexpected end of file.");
	
	//check if the next token contains any character that is not a number: if it has not, it is a port
	std::string currentToken = _tokens[_currentToken];
	if (allDigits(currentToken))
		validatePort(currentToken, server);

	//check if format indicates an host:port
	size_t pos = currentToken.find(':');
	if (pos != std::string::npos)
	{
		std::string host = currentToken.substr(0, pos);
		validateHost(host, server);
		std::string port = currentToken.substr(pos + 1);
		validatePort(port, server);
	}

	//ELSE TREAT IT LIKE HOST ONLY????
	
	//verify if it is host only or host and port
	// can be listen 80; listen localhost; listen 127.0.0.1; listen 127.0.0.1:8080

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