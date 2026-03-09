#include "ConfigParser.hpp"

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
	std::string root = object.getRoot();
	if (!root.empty())
		throw ConfigException("Error: Root was already defined.");

	++_currentToken;
	checkIfTokenExists();

	std::string rootPath = _tokens[_currentToken];

	if (rootPath == ";")
		throw ConfigException("Error: There must be a valid path after 'root' keyword.");

	ConfigUtils::validatePath(rootPath);

	//there can only be one token between the 'root' word and the ';', because the path can't have spaces in between
	if (_currentToken + 1 >= _tokens.size())
		throw ConfigException("Error: Unexpected end of file after " + _tokens[_currentToken - 1]);
	if (_tokens[_currentToken + 1] != ";")
		throw ConfigException("Error: Unknown path detected after 'root'.");

	object.setRoot(rootPath);
}