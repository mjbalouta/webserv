#include "ConfigUtils.hpp"

/**
 * @brief Validations for host
 * 
 * @param token 
 * @param server 
 */
void ConfigUtils::validateHost(std::string& token, ServerConfig& server)
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
		validateIP(token, errorMessage);
	if (isHostname)
		validateHostname(token, errorMessage);
	server.setHost(token);
}

/**
 * @brief Validations for port
 * 
 * @param token 
 * @param server 
 */
void ConfigUtils::validatePort(std::string& token, ServerConfig& server)
{
	int port = atoi(token.c_str());
	if (port < 1 || port > 65535)
		throw ConfigException("Error: Invalid port " + token);
	server.setPort(port); 
}


/**
 * @brief Checks the Hostname format
 * 
 * @param token 
 * @param errorMessage 
 */
void validateHostname(std::string& token, std::string& errorMessage)
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

/**
 * @brief Checks if the IP format is a valid one
 * 
 * @param token 
 * @param errorMessage 
 */
void validateIP(std::string& token, std::string& errorMessage)
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
