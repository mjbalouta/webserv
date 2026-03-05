#pragma once

//#include "../Includes.hpp"
#include "Includes.hpp"

class PathResolver{
    public:
        std::string normalizePath(const std::string& path);
        bool isPathSafe(const std::string& requested, const std::string& root);
//        std::string resolveToFilesystem(const std::string& uriPath, const std::string& documentRoot);
};