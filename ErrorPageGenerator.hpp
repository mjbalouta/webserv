#include "Includes.hpp"

class ErrorPageGenerator {
public:
    std::string generateErrorPage(int statusCode, const std::string& message);
    std::string loadCustomErrorPage(int statusCode, const ServerConfig& config);
};