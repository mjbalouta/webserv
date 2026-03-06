#include "ConfigParser.hpp"

/**
 * @brief After finding the server block, it creates a ServerConfig
 * object and stores everything inside the block inside this object
 * 
 */
void ConfigParser::parseServer()
{
	ServerConfig server;

	while (++_currentLine < _lines.size())
	{
		std::string line = _lines[_currentLine];
		line = trim(line);

		if (line.empty())
			continue;
		
		//end of the block
		if (line == "}")
			break;

		if (line.find("location") == 0)
			parseLocation();
		else if (line.find("listen") == 0)
			parseListen();
		else if (line.find("root") == 0)
			parseRoot();
			
	}
}