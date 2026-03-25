#include "FileSystemHandler.hpp"

/**
 * @brief Check if a path exists using stat system call. It returns true if the path exists and false otherwise.
 * 
 * @param path 
 * @return true 
 * @return false 
 */

bool FileSystemHandler::pathExists(const std::string& path){
    if (path.empty())
        return false;
    struct stat st;
//  struct stat verifies if the path is a valid one
    if (stat(path.c_str(), &st) == 0)
        return true;
    return false;
}


/**
 * @brief Check if a path is a directory using stat system call. 
 * It returns true if the path is a directory and false otherwise.
 * 
 * @param path 
 * @return true 
 * @return false 
 */
bool FileSystemHandler::isDirectory(const std::string& path) {
    if (path.empty())
        return false;
    struct stat st;
//  struct stat verifies if the path is a directory
    if (stat(path.c_str(), &st) == 0)
        return S_ISDIR(st.st_mode);
    return false;
}


/**
 * @brief Check if a path is readable using access system call. 
 * It returns true if the path is readable and false otherwise.
 * 
 * @param path 
 * @return true 
 * @return false 
 */
bool FileSystemHandler::isReadable(const std::string& path){
    if (path.empty())
        return false;
//  access verifies READ permissions
    if (access(path.c_str(), R_OK) == 0)
        return true;
    return false;
}

bool FileSystemHandler::isWritable(const std::string& path){
    if (path.empty())
        return false;
    if (access(path.c_str(), W_OK) == 0)
        return true;
    return false;
}


/**
 * @brief Read the content of a file and return it as a string.
 * 
 * @param path 
 * @return std::string 
 */
std::string FileSystemHandler::readFile(const std::string& path, size_t maxSize){
    if (path.empty())
        throw std::runtime_error("No file path");
    if (!isReadable(path))
        throw std::runtime_error("File is not readable");
    size_t size = getFileSize(path);
    if (size > maxSize)
        throw std::runtime_error("File size exceeds maximum allowed size");

    std::ifstream readFile(path.c_str(), std::ios::in | std::ios::binary);
    if (!readFile)
        throw std::runtime_error("Cannot open file");

    std::string content;
    content.resize(size);
    if (size > 0)
        readFile.read(&content[0], size);
    if (!readFile && size > 0)
        throw std::runtime_error("Failed to read file");
    return content;
}


/**
 * @brief List the entries of a directory and return them as a vector of strings.
 * 
 * @param path 
 * @return std::vector<std::string> 
 */
std::vector<std::string> FileSystemHandler::listDirectory(const std::string& path){
    std::vector<std::string> dirEntries;

    if (path.empty())
        return dirEntries;
    
    if (!isDirectory(path))
        return dirEntries;

    DIR *dp = opendir(path.c_str());
    if (dp == NULL)
        return dirEntries;

    struct dirent *dirp;
    while ((dirp = readdir(dp)) != NULL)
    {
        std::string entryName = dirp->d_name;
        if (entryName == "." || entryName == "..")
            continue;
        dirEntries.push_back(entryName);
    }
    closedir(dp);
    
    return dirEntries;
}


/**
 * @brief Get the size of a file in bytes.
 * 
 * @param path 
 * @return size_t 
 */
size_t FileSystemHandler::getFileSize(const std::string& path){
    if (path.empty())
        return 0;
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return 0;
    if (!S_ISREG(st.st_mode))
        return 0;
    return static_cast<size_t>(st.st_size);
}

std::time_t FileSystemHandler::getLastMODTime(const std::string& path){
    if (path.empty())
        return 0;
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        return st.st_mtime;
    }
    return 0;
}

bool FileSystemHandler::writeFile(const std::string& path, const std::string& content)
{
    if (path.empty())
        return false;

    std::ofstream out(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    if (!content.empty())
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(out);
}

bool FileSystemHandler::removeFile(const std::string& path)
{
    if (path.empty())
        return false;
    return (::unlink(path.c_str()) == 0);
}

bool FileSystemHandler::removeDirectory(const std::string& path){
    if (path.empty())
        return false;
    return (::rmdir(path.c_str()) == 0);
}

bool FileSystemHandler::isMultipartFormData(const std::string& contentTypeHeader){
    std::string lowerContentType = toLower(contentTypeHeader);
    return (lowerContentType.find("multipart/form-data") != std::string::npos);
}

std::string FileSystemHandler::extractMultipartBoundary(const std::string& contentTypeHeader){
    size_t boundaryPos = contentTypeHeader.find("boundary=");
    if (boundaryPos != std::string::npos)
    {
        std::string value = contentTypeHeader.substr(boundaryPos + 9);
        // Trim at next parameter separator.
        size_t semi = value.find(';');
        if (semi != std::string::npos)
            value = value.substr(0, semi);
        // Trim surrounding whitespace.
        value = trimSpaces(value);
        // Strip optional quotes.
        if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
            value = value.substr(1, value.size() - 2);
        value = trimSpaces(value);
        return value;
    }
    return "";
}