#pragma once

#include "Includes.hpp"

class MimeTypeResolver {
    public:
        MimeTypeResolver();
        ~MimeTypeResolver() {};
        std::string getTypeByExtension(const std::string& filename);
    private:
        std::map<std::string, std::string> mimeTypes;
};