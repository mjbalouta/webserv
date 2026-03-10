#pragma once

#include "Includes.hpp"

class ConfigUtils
{
	public:
	static void validateHost(std::string& token, ServerConfig& server);
	static void validatePort(std::string& token, ServerConfig& server);
	static void validateIP(std::string& token, std::string& errorMessage);
	static void validateHostname(std::string& token, std::string& errorMessage);
	static void validateFilename(std::string& token);
	static void validateStatusCode(std::string& token);
	static void	validateErrorPagePath(std::string& token);
	static void validatePath(std::string& token);
	static unsigned long calculateSize(unsigned long size, int option);
	static void validateURL(std::string& url);
	static void validateMessage(std::string& token);
};