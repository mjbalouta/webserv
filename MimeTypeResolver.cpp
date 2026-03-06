#include "MimeTypeResolver.hpp"


/**
 * @brief Construct a new Mime Type Resolver:: Mime Type Resolver object
 * 
 */
MimeTypeResolver::MimeTypeResolver() {
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".zip"] = "application/zip";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".ogg"] = "audio/ogg";
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".webm"] = "video/webm";
    // Add more as needed
}

/**
 * @brief Get the MIME type based on the file extension.
 * 
 * @param filename 
 * @return std::string filename is the name of the file for which we want to determine the MIME type.
 *         The function extracts the file extension from the filename, converts it to lowercase, and looks it up in the mimeTypes map. 
 *         If a matching MIME type is found, it returns that; otherwise, it returns "application/octet-stream" as a default for unknown types.
 */
std::string MimeTypeResolver::getTypeByExtension(const std::string& filename){
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos)
            return "application/octet-stream";
    std::string extension = filename.substr(dot);

    std::cout << "Extension: " << extension << std::endl;
    for (size_t i = 0; i < extension.size(); ++i)
        extension[i] = std::tolower(extension[i]);

    std::map<std::string, std::string>::iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end())
        return it->second;
    return "application/octet-stream";    
}