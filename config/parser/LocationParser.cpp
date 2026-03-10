#include "ConfigParser.hpp"

/**
 * @brief Validates information after 'return' keyword
 * (Return function: when a return is triggered, the server stops looking for files and immediately
 * sends a response to the client.)
 * 
 * @param location 
 */
void ConfigParser::parseReturn(LocationConfig& location)
{
	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definitions after 'return' keyword.");

	if (!allDigits(token))
		throw ConfigException("Error: Missing HTTP status code after 'return' keyword.");
	if (token.size() != 3)
		throw ConfigException("Error: Invalid HTTP status code format: " + token);
	int code = atoi(token.c_str());
	location.setReturnStatusCode(code);
	++_currentToken;
	checkIfTokenExists();
	//codes inside the range 300-399 must have an URL next to it
	if (code >= 300 && code <= 399)
	{
		if (_tokens[_currentToken] == ";")
			throw ConfigException("Error: Missing URL for status code: " + _tokens[_currentToken - 1]);
		ConfigUtils::validateURL(_tokens[_currentToken]);
		location.setReturnURL(_tokens[_currentToken]);
		++_currentToken;
	}
	//codes outside that range can have a message next to it or not
	else if (_tokens[_currentToken] != ";")
	{
		std::string message;
		while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
		{
			ConfigUtils::validateMessage(_tokens[_currentToken]);
			if (!message.empty())
				message += " ";
			message += _tokens[_currentToken];
			++_currentToken;
		}
		location.setReturnMessage(message);
	}
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'return' definition.");
}

/**
 * @brief Checks information after 'allow_methods' keyword
 * 
 * @param location 
 */
void ConfigParser::parseAllowMethods(LocationConfig& location)
{
	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definitions after 'allow_methods' keyword.");

	while(_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		token = _tokens[_currentToken];
		if (token != "GET" && token != "POST" && token != "DELETE")
			throw ConfigException("Error: Invalid method: " + token);
		location.addAllowedMethod(token);
		++_currentToken;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'allow_methods' definition.");
}

/**
 * @brief Checks information after 'alias' token
 * 
 * @param location 
 */
void ConfigParser::parseAlias(LocationConfig& location)
{
	//check if root is defined (they cannot exist both)
	if (!location.getRoot().empty())
		throw ConfigException("Error: Alias cannot be defined when root already exists.");
	//check if alias already exists
	if (location.getAliasFlag() == true)
		throw ConfigException("Error: Alias cannot be redefined.");

	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definition after 'alias' keyword.");

	ConfigUtils::validatePath(token);
	location.setAlias(token);
	location.setAliasFlag(true);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after alias definition.");
}

/**
 * @brief Main function to parse location blocks
 * 
 * @param server 
 */
void ConfigParser::parseLocation(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	//mandatory format: 'location' + 'path' + {
	if (token == "{")
		throw ConfigException("Error: Expected a path after 'location' keyword.");

	ConfigUtils::validatePath(token);
	LocationConfig location(token);

	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] != "{")
		throw ConfigException("Error: Expected '{' token after location's path.");

	int endBracket = 0;
	while (++_currentToken < _tokens.size())
	{
		std::string token = _tokens[_currentToken];
		if (token == "}")
		{
			endBracket = 1;
			break;
		}
		else if (token == "root")
			parseRoot(location);
		else if (token == "alias")
			parseAlias(location);
		else if (token == "allow_methods")
			parseAllowMethods(location);
		else if (token == "client_max_body_size")
			parseMaxBodySize(location);
		else if (token == "index")
			parseIndex(location);
		else if (token == "autoindex")
			parseAutoindex(location);
		else if (token == "return")
			parseReturn(location);
	/*	else if (token == "cgi_pass") or cgi_ext?? ver sobre isto
			parseCGI(location);
		else if (token == "error_page")
			parseErrorPage(location);
		else if (token == "upload_store")
			parseUploadStore(location);		
	*/
		else
			throw ConfigException("Error: Unknown keyword " + _tokens[_currentToken]);
	}
	if (!endBracket)
		throw ConfigException("Error: Expected '}' in the end of location block.");

}