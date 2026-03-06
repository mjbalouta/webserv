#pragma once

#include "Includes.hpp"

class MimeTypeResolver {
    public:
        static std::string getMimeType(const std::string& extension);
        std::string getTypeByExtension(const std::string& filename);
};