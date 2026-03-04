#include "Utils.hpp"
#include <cctype>
#include <sstream>
#include <cstdlib>

// Check if character is whitespace
bool isWhitespace(char c) {
	return std::isspace((unsigned char)c);
}

// Trim leading and trailing whitespace
std::string trim(const std::string &s) {
	size_t start = 0;
	while (start < s.size() && isWhitespace(s[start]))
		++start;
	size_t end = s.size();
	while (end > start && isWhitespace(s[end - 1]))
		--end;
	return s.substr(start, end - start);
}

// Convert string to lowercase
std::string toLower(const std::string &s) {
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i) {
		out[i] = static_cast<char>(std::tolower((unsigned char)out[i]));
	}
	return out;
}

// Convert string to uppercase
std::string toUpper(const std::string &s) {
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i) {
		out[i] = static_cast<char>(std::toupper((unsigned char)out[i]));
	}
	return out;
}

// Simple heuristic to detect TLS/HTTPS ClientHello bytes
bool looks_like_tls(const char *buf, int len) {
	if (len < 3)
		return false;
	unsigned char b0 = static_cast<unsigned char>(buf[0]);
	unsigned char b1 = static_cast<unsigned char>(buf[1]);
	// TLS record: ContentType(0x16) + Version(0x03, 0x01/0x02/0x03)
	if (b0 == 0x16 && b1 == 0x03)
		return true;
	return false;
}

// Convert string to int (safe wrapper around atoi)
int stringToInt(const std::string &s) {
	if (s.empty())
		return 0;
	return std::atoi(s.c_str());
}