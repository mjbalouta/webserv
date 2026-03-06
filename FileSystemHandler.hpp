#pragma once

#include "Includes.hpp"

class FileSystemHandler{
    public:
        bool pathExists(const std::string& path);
        bool isDirectory(const std::string& path);
        bool isReadable(const std::string& path);
        std::string readFile(const std::string& path);
         std::vector<std::string> listDirectory(const std::string& path);
        size_t getFileSize(const std::string& path);
        std::time_t getLastMODTime(const std::string& path);
};