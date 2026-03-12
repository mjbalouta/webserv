#include "ConfigParser.hpp"

/**
 * @brief Validates information after 'error_page' keyword
 * 
 * @param server 
 */
template <typename T>
void ConfigParser::parseErrorPage(T& object)
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
		object.addErrorPages(*it, path);

}

/**
 * @brief Validates information after 'autoindex' keyword
 * 
 * @param server 
 */
template <typename T>
void ConfigParser::parseAutoindex(T& object)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'autoindex'.");

	if (_tokens[_currentToken] == "on")
		object.setAutoIndex(true);
	else if (_tokens[_currentToken] == "off")
		object.setAutoIndex(false);
	else
		throw ConfigException("Error: Invalid definition of autoindex: " + _tokens[_currentToken]);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after autoindex definition.");
}

/**
 * @brief Validates information after 'index' keyword
 * 
 * @param object 
 */
template <typename T>
void ConfigParser::parseIndex(T& object)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definition after keyword 'index'.");

	//if there are more than one definition of 'index', we must clear the previous one
	//because the last definition should overwrite any previous one
	object.clearIndexes();

	while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		std::string token = _tokens[_currentToken];
		ConfigUtils::validateFilename(token);
		object.addIndex(token);
		_currentToken++;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after index definitions.");
}

/**
 * @brief Validates information after 'root' keyword
 * (being a template method, it works for server and location objects)
 * 
 * @tparam T 
 * @param object
 */
template <typename T>
void ConfigParser::parseRoot(T& object)
{
	if (object.getRootFlag() == true)
		throw ConfigException("Error: Root was already defined.");
	if (object.getAliasFlag() == true)
		throw ConfigException("Error: Root and alias cannot coexist in a location block.");

	++_currentToken;
	checkIfTokenExists();

	std::string rootPath = _tokens[_currentToken];

	if (rootPath == ";")
		throw ConfigException("Error: There must be a valid path after 'root' keyword.");

	ConfigUtils::validatePath(rootPath);
	object.setRoot(rootPath);
	object.setRootFlag(true);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Unknown path detected after 'root'.");
}

/**
 * @brief Validates information after 'client_max_body_size' keyword
 * 
 * @param server 
 */
template <typename T>
void ConfigParser::parseMaxBodySize(T& object)
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

	object.setMaxBodySize(max);
}