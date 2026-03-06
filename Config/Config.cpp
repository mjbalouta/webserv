#include "Config.hpp"

Config::Config() {}

Config::~Config() {}

std::vector<std::string> Config::getPort() const {
	std::vector<std::string> ports;
	ports.push_back("8080");
	return ports;
}

std::vector<std::string> Config::getHost() const {
	std::vector<std::string> hosts;
	hosts.push_back("127.0.0.1");
	return hosts;
}

std::vector<std::string> Config::getServerName() const {
	std::vector<std::string> names;
	names.push_back("localhost");
	return names;
}

std::vector<std::string> Config::getClientMaxBodySize() const {
	std::vector<std::string> sizes;
	sizes.push_back("1048576");
	return sizes;
}

std::vector<std::string> Config::getDefaultRoot() const {
	std::vector<std::string> roots;
	roots.push_back("/var/www/html");
	return roots;
}

std::vector<Config> Config::getConfig() const {
	std::vector<Config> configs;
	configs.push_back(*this);
	return configs;
}
