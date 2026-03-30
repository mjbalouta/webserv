#pragma once

#include "Exception.hpp"
#include "../ServerConfig.hpp"
#include "../../Utils.hpp"
#include "../../fileResourceManagement/FileSystemHandler.hpp"

class ServerConfig;

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
	static void checkExtension(std::string& token);
	static void checksIfAlreadyExists(std::string& token, LocationConfig& location);
	static void checkIfDirectory(std::string& token);
	static void checkIfPortExists(std::string& token, ServerConfig& server);
	static void CheckInterpreter(const std::string& extensionPath);
};