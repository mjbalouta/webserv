#pragma once
#include "Includes.hpp"

// Utility functions for the webserver

// String manipulation
std::string trim(const std::string &s);
std::string toLower(const std::string &s);
std::string toUpper(const std::string &s);

// HTTP/Network helpers
bool looks_like_tls(const char *buf, int len);
bool isWhitespace(char c);

// Conversion helpers
int stringToInt(const std::string &s);