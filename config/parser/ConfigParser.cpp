#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string& filename)
: _filename(filename) {}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return _servers;
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
		throw FileException("Unable to open config file.");

	std::string line;
	while (std::getline(file, line))
		_lines.push_back(line);
	
	_currentLine = 0;

	while(_currentLine < _lines.size())
	{
		std::string line = _lines[_currentLine];
		line = trim(line);

		if (line.empty())
		{
			_currentLine++;
			continue;
		}

		if (line.find("server") == 0)
			parseServer();
		else
			throw ConfigException("Invalid configuration outside server block.");
	}
}