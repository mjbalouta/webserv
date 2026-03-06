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
    if (getFileSize(path) > maxSize)
        throw std::runtime_error("File size exceeds maximum allowed size");
    std::string fileContent;
    std::string line;

// Read from the text file
    std::ifstream readFile(path.c_str(), std::ios::binary);

    if (!readFile)
        throw std::runtime_error("Cannot open file");

// Use a while loop together with the getline() function to read the file line by line
    while (getline(readFile, line)) {
        fileContent += line + "\n";
    }

// Close the file
    readFile.close();
    return fileContent;
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
    std::ifstream file(path);
    return (file.tellg());
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