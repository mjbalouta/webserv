#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string& filename)
: _filename(filename) {}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return _servers;
}

/**
 * @brief Separates each word of the config file by a space and pushes it into
 * container of tokens
 * 
 */
void ConfigParser::tokenize(std::string content)
{
	std::string spacedContent = "";

	//build a new string with added spaces around special characters
	for (size_t i = 0; i < content.length(); i++)
	{
		if (content[i] == '{' || content[i] == '}' || content[i] == ';')
		{
			spacedContent += " ";
			spacedContent += content[i];
			spacedContent += " ";
		}
		else
			spacedContent += content[i];
	}

	//add tokens to the container
	std::stringstream ss(spacedContent);
	std::string word;
	//>> extracts the next chunk of characters until it hits a space
	while (ss >> word)
	{
		if (word.at(0) == '#')
		{
			//skips all the text in a line of comment
			ss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}
		_tokens.push_back(word);
	}
}


/**
 * @brief Opens the config file, stores each line in _lines and then goes
 * through the container and checks if the first word is 'server'
 * 
 */
void ConfigParser::parse()
{
	std::ifstream file(_filename.c_str());
	if (!file.is_open())
		throw FileException("Error: Unable to open config file.");

	//read the whole file into a string
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	//separate each token by spaces
	tokenize(content);

	//track if first token is 'server'
	_currentToken = 0;
	while (_currentToken < _tokens.size())
	{
		if (_tokens[_currentToken] == "server")
		{
			_currentToken++;
			parseServer();
		}
		else
			throw ConfigException("Error: Unexpected keyword: " + _tokens[_currentToken]);
	}
	
}
