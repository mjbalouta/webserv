#include "PathResolver.hpp"

/**
 * @brief Path normalization function that takes a path string and normalizes it 
 *  by removing redundant slashes, resolving "." and ".." components, and ensuring a clean path structure.
 * 
 * @param path 
 * @return std::string 
 */

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

/**
 * @brief Path checker to see if the requested path is safe to access based on the root directory.
 *  It normalizes both the root and the requested path, combines them, and checks if the combined path starts with the normalized root path.
 *  If the root is empty, it considers all paths as safe.
 * 
 * @param requested 
 * @param root
 * @return true 
 * @return false 
 */

bool PathResolver::isPathSafe(const std::string& requested, const std::string& root){
    PathResolver resolver;

//    std::string normalizedRequest;
    std::string normalizedRoot;
    std::string combined = root;
    
    normalizedRoot = resolver.normalizePath(root);
/*    normalizedRequest = resolver.normalizePath(requested);
    
    if (!normalizedRoot.empty())
        combined = normalizedRoot + "/" + normalizedRequest;
    else
        combined = normalizedRequest;
    
    combined = resolver.normalizePath(combined); */

    if (!root.empty())
        combined = root + "/" + requested;
    else
        combined = requested;

    combined = resolver.normalizePath(combined);

    std::vector<std::string> vecRoot;
    std::stringstream splitRoot(normalizedRoot);
    std::vector<std::string> vecRequestFullPath;
    std::stringstream splitRequest(combined);
    std::string temp;
    
    while (getline(splitRoot, temp, '/'))
        if (!temp.empty())
            vecRoot.push_back(temp);

    while (getline(splitRequest, temp, '/'))
        if (!temp.empty())
            vecRequestFullPath.push_back(temp);

    if (vecRoot.empty())
        return true;

    if (vecRequestFullPath.size() < vecRoot.size())
        return false;

    std::vector<std::string>::iterator itRoot = vecRoot.begin();
    std::vector<std::string>::iterator itRequest = vecRequestFullPath.begin();
    for (; itRoot != vecRoot.end(); itRoot++, itRequest++)
    {
        if (*itRoot != *itRequest)
            return false;
    }
    return true;
}

std::string PathResolver::resolveToFilesystem(const std::string& uriPath, const std::string& documentRoot) {
    PathResolver resolver;
    if (resolver.isPathSafe(uriPath, documentRoot)){
        std::string combined;
        if (!documentRoot.empty())
            combined = documentRoot + "/" + uriPath;
        else
            combined = uriPath;
        combined = resolver.normalizePath(combined);
        return combined;
    }
    return "";
}
