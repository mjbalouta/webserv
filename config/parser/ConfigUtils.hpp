#pragma once

#include "Includes.hpp"

class ConfigUtils
{
	public:
	static void validateHost(std::string& token, ServerConfig& server);
	static void validatePort(std::string& token, ServerConfig& server);

};