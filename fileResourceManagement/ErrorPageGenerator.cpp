#include "ErrorPageGenerator.hpp"

/**
 * @brief Generate a simple HTML error page based on the provided status code and message.
 * The generated page includes a title, a heading with the status code and message, and a link to return to the home page.
 * 
 * @param statusCode 
 * @param message 
 * @return std::string 
 */

std::string ErrorPageGenerator::getReasonPhrase(int status_code){
    switch (status_code)
    {
        case 100: return ("Continue");
        case 101: return ("Switching Protocols");
        case 200: return ("OK");
        case 201: return ("Created");
        case 202: return ("Accepted");
        case 203: return ("Non-Authoritative Information");
        case 204: return ("No Content");
        case 205: return ("Reset Content");
        case 206: return ("Partial Content");
        case 300: return ("Multiple Choices");
        case 301: return ("Moved Permanently");
        case 302: return ("Found");
        case 303: return ("See Other");
        case 304: return ("Not Modified");
        case 305: return ("Use Proxy");
        case 307: return ("Temporary Redirect");
        case 400: return ("Bad Request");
        case 401: return ("Unauthorized");
        case 402: return ("Payment Required");
        case 403: return ("Forbidden");
        case 404: return ("Not Found");
        case 405: return ("Method Not Allowed");
        case 406: return ("Not Acceptable");
        case 407: return ("Proxy Authentication Required");
        case 408: return ("Request Timeout");
        case 409: return ("Conflict");
        case 410: return ("Gone");
        case 411: return ("Length Required");
        case 412: return ("Precondition Failed");
        case 413: return ("Payload Too Large");
        case 414: return ("URI Too Long");
        case 415: return ("Unsupported Media Type");
        case 416: return ("Range Not Satisfiable");
        case 417: return ("Expectation Failed");
        case 500: return ("Internal Server Error");
        case 501: return ("Not Implemented");
        case 502: return ("Bad Gateway");
        case 503: return ("Service Unavailable");
        case 504: return ("Gateway Timeout");
        case 505: return ("HTTP Version Not Supported");

        default: return ("");
    }
}

std::string ErrorPageGenerator::generateErrorPage(int statusCode, const std::string& message) {
    std::string reasonPhrase = getReasonPhrase(statusCode);
	std::stringstream ss;
	ss << statusCode;
	std::string codeStr = ss.str();
    std::string html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>" + codeStr + " " + reasonPhrase + "</title>";
    html += "</head>";
    html += "<body>";
    html += "<h1>" + codeStr + " " + reasonPhrase + "</h1>";
    html += "<p>" + message + "</p>";
    html += "<a href=\"/\">Return to Home</a>";
    html += "</body>";
    html += "</html>";

    return html;
}

std::string ErrorPageGenerator::loadCustomErrorPage(int statusCode, const ConfigResolved& config){

    std::map<int, std::string> errorPages = config.getErrorPages();
    std::map<int, std::string>::const_iterator pageIt = errorPages.find(statusCode);
    if (pageIt != errorPages.end())
    {
		std::string filePath;
		std::string root = config.getRoot();
		if (!root.empty() && root[0] == '/')
			filePath = root + pageIt->second;
		else
			filePath = config.getAbsolutePath() + root + pageIt->second;
		std::ifstream file(filePath.c_str());
        if (file.is_open()){
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            return content;
        }
    }
    return generateErrorPage(statusCode, getReasonPhrase(statusCode)); // Fallback para a página de erro genérica se não houver uma personalizada
}