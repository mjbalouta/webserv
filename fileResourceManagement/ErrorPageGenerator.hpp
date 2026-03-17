#pragma once

#include "../Includes.hpp"
#include "../routing/ConfigResolved.hpp"

class ErrorPageGenerator {
public:
    std::string generateErrorPage(int statusCode, const std::string& message);
    std::string loadCustomErrorPage(int statusCode, const ConfigResolved& config);
    std::string getReasonPhrase(int status_code);
};