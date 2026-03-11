#include "ConfigUtils.hpp"

/**
 * @brief Checks if the path is a directory
 * 
 * @param token 
 */
void ConfigUtils::checkIfDirectory(std::string& token)
{
	validatePath(token);
	if (token.find("//") != std::string::npos)
		throw ConfigException("Error: Invalid path format: " + token);
}

/**
 * @brief Goes through the cgi container and checks if the extension already exists
 * 
 * @param token 
 */
void ConfigUtils::checksIfAlreadyExists(std::string& token, LocationConfig& location)
{
	std::map<std::string, std::string> temp = location.getCGI();
	std::map<std::string, std::string>::iterator it = temp.find(".py");
	if (it != temp.end())
		throw ConfigException("Error: Extension already defined: " + token);
}

/**
 * @brief Validates the extension format
 * 
 * @param token 
 */
void ConfigUtils::checkExtension(std::string& token)
{
	if (token.size() < 2) //menor do que 2 ou 3? aceitamos .c?
		throw ConfigException("Error: Wrong extension format: " + token);
	if (token[0] != '.')
		throw ConfigException("Error: Wrong extension format: " + token);
	if (token.find('.', 1) != std::string::npos)
		throw ConfigException("Error: Wrong extension format: " + token);
	
}

/**
 * @brief Validates the message format after a status code in return
 * 
 * @param token 
 */
void ConfigUtils::validateMessage(std::string& token)
{
	if (token.find("{}#") != std::string::npos)
		throw ConfigException("Error: Message contains invalid characters: " + token);
}

/**
 * @brief Validates an URL format
 * 
 * @param url 
 */
void ConfigUtils::validateURL(std::string& url)
{
	if (url.find_first_of("{};#") != std::string::npos)
		throw ConfigException("Error: Invalid url format: " + url);

	if (url.find("http://") != 0 && url.find("https://") != 0 && url[0] != '/')
		throw ConfigException("Error: Invalid url format: " + url);	
}

/**
 * @brief Validates path format
 * 
 * @param token
 */
void ConfigUtils::validatePath(std::string& token)
{
	if (token[0] != '/')
		throw ConfigException("Error: Invalid path format: " + token);

	if (token.find("..") != std::string::npos)
		throw ConfigException("Error: Invalid path format: " + token);
	
	if (token.find_first_of("{};#*?|") != std::string::npos)
		throw ConfigException("Error: Invalid characters in path: " + token);
}

/**
 * @brief Calculates the conversion for 'max_body_size'
 * 
 * @param token 
 * @param option 
 */
unsigned long ConfigUtils::calculateSize(unsigned long size, int option)
{
	unsigned long result = size;

	switch (option)
	{
	case 1: // Kilo
		result = size * 1024;
		break;
	case 2: // Mega
		result = size * 1024 * 1024;
		break;
	case 3: // Giga
		result = size * 1024 * 1024;
		break;
	default: 
		break;
	}

	return result;
}

/**
 * @brief Checks if the format of a path is a valid one for an error page
 * 
 * @param token 
 */
void ConfigUtils::validateErrorPagePath(std::string& token)
{
	if (token.empty() || token[0] != '/')
		throw ConfigException("Error: Invalid path format: " + token);

	if (token == "/")
		throw ConfigException("Error: Invalid path format: " + token);

	if (token.find_first_of("{};#*?|") != std::string::npos)
		throw ConfigException("Error: Invalid characters: " + token);

	if (token.find("..") != std::string::npos)
		throw ConfigException("Error: Invalid path format: " + token);

	if (token[token.length() - 1] == '/')
		throw ConfigException("Error: Path must lead to a file, not a directory: " + token);
}

/**
 * @brief Checks if status code is a valid one
 * 
 * @param token 
 */
void ConfigUtils::validateStatusCode(std::string& token)
{
	if (token.size() != 3)
		throw ConfigException("Error: Invalid status code: " + token);

	if (!allDigits(token))
		throw ConfigException("Error: Invalid status code: " + token);

	int code = atoi(token.c_str());
	//valids HTTP status codes for nginx
	if (code < 300 || code > 599)
		throw ConfigException("Error: Invalid status code: " + token);
}

/**
 * @brief Checks if filename format is correct
 * 
 * @param token 
 */
void ConfigUtils::validateFilename(std::string& token)
{
	if (token.find('/') != std::string::npos)
		throw ConfigException("Error: Invalid filename in index: " + token);
	
	if (token == ".." || token == ".")
		throw ConfigException("Error: Invalid filename in index: " + token);

	if (token.find_first_of("{};#") != std::string::npos)
		throw ConfigException("Error: Invalid filename in index: " + token);
}

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
void ConfigUtils::validateHostname(std::string& token, std::string& errorMessage)
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
void ConfigUtils::validateIP(std::string& token, std::string& errorMessage)
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
