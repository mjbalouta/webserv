#include "PathResolver.hpp"

/*
    * This function normalizes the path by removing redundant slashes and resolving any "." or ".." components.
    * It splits the input path by '/' and processes each component to construct a normalized path.
    * The resulting normalized path is returned as a string.
    * 
    * Handling only . and .. , need to make sure thats enough or need to handle special characters as well
    * 
    * */

std::string PathResolver::normalizePath(const std::string& path){
    std::string normalizedPath;
    std::stringstream split(path);
    std::string temp;
    std::vector<std::string> array;

// Split by /    
    while (getline(split, temp, '/'))
        array.push_back(temp);

    std::vector<std::string>::iterator it = array.begin();
    for (; it != array.end(); ++it) {
        if (*it == "" || *it == ".")
            continue;
        else if (*it == "..") {
            if (!normalizedPath.empty())
            {
                size_t lastSlash = normalizedPath.find_last_of('/');
                if (lastSlash != std::string::npos)
                    normalizedPath.erase(lastSlash);
                else
                    normalizedPath.clear();
            }
        }
        else {
            if (!normalizedPath.empty() && (*(normalizedPath.end() - 1) != '/'))
                normalizedPath += '/';
            normalizedPath += *it;
        }
    }
    return normalizedPath;
}

bool isPathSafe(const std::string& requested, const std::string& root);

