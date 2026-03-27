#pragma once
/* 
#include "../Includes.hpp"
#include "../Utils.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../ServerManager/Request.hpp"
#include "../fileResourceManagement/PathResolver.hpp"
#include "../fileResourceManagement/FileSystemHandler.hpp"
#include "../fileResourceManagement/MimeTypeResolver.hpp"
#include "../fileResourceManagement/ErrorPageGenerator.hpp"
#include "../routing/ConfigResolved.hpp"
#include <algorithm>

class CGIHandler {
    private:

    public:
        std::string requestHeadtoCGIEnv(const std::string& header);
        char** buildCGIEnv(const Request& request, const ConfigResolved& resolvedConfig, const std::string& scriptPath);
        bool isCgiRequest(const Request& request, const ConfigResolved& resolvedConfig, std::string& outScriptPath, std::string& outExecutor);
}; */