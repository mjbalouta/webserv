#include "ConfigParser.hpp"

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
	while (++_currentToken <= _tokens.size())
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
		//CHECK IF I HAVE TO ADD ANYTHING ELSE HERE
		else
			throw ConfigException("Error: Unknown keyword " + _tokens[_currentToken]);
	}

	if (!endBracket)
		throw ConfigException("Error: Expected '}' in the end of location block.");

}