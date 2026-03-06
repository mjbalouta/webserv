#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const std::string& filename)
: _filename(filename) {}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return _servers;
}

void ConfigParser::parse()
{
	
}