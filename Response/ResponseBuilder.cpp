#include "ResponseBuilder.hpp"

std::string ResponseBuilder::returnResponse(const Request& request, const ServerConfig& config) {
	std::vector<LocationConfig> locations = config.getLocations();
	// Determine the appropriate location block based on the request path
	// This is a simplified example and should be expanded to handle more complex scenarios
	const LocationConfig* matchedLocation = NULL;
	for (size_t i = 0; i < locations.size(); ++i) 
	{
		std::string requestpath = request.getPath();
		PathResolver resolver;
		std::string locationRoot;
		if (locations[i].getRoot().empty())
			locationRoot = config.getRoot();
		else
			locationRoot = locations[i].getRoot();
		if (resolver.isPathSafe(requestpath, locationRoot) == true) {
			if (requestpath.find(locations[i].getPath()) == 0)
			{
				if (matchedLocation != NULL)
				{
					if (matchedLocation->getPath().length() < locations[i].getPath().length())
						matchedLocation = &locations[i];
				}
				else
					matchedLocation = &locations[i];
			}
		}
	}
	if (matchedLocation == NULL)
		return returnErrorResponse(404, request, config);
	else
	{
		std::vector<std::string> methods = matchedLocation->getAllowedMethods();
		std::vector<std::string>::iterator it = methods.begin();
		bool Allowed = false;
		for (; it != methods.end(); it++)
			if (*it == request.getMethodStr())
				Allowed = true;
		if (!Allowed)
			return returnErrorResponse(405, request, config);
		
	}
	std::string response;
	return response; // Placeholder response
}

std::string ResponseBuilder::returnErrorResponse(int statusCode, const Request& request, const ServerConfig& config){
		_statusCode = statusCode;
		std::stringstream ss;
		ss << _statusCode;
		_statusLine = request.getVersion() + " " + ss.str() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_contentType = "text/html";
		std::string errorPage = error.loadCustomErrorPage(_statusCode, config);
		if (errorPage.empty())
			errorPage = error.generateErrorPage(_statusCode, error.getReasonPhrase(_statusCode));
		_body = errorPage;
		_contentLength = _body.size();
		std::string response = _statusLine;
		setStandardHeaders(response, _contentType, _contentLength);
		response += "\r\n" + _body;
		return response;
}

void ResponseBuilder::setStandardHeaders(std::string& response, const std::string& contentType, size_t contentLength) {
	response += "Content-Type: " + contentType + "\r\n";
	response += "Content-Length: " + std::to_string(contentLength) + "\r\n";
	response += "Date: " + getDateString() + "\r\n";
	response += "Last-Modified: " + getLastModifiedString() + "\r\n";
	response += "Connection: close\r\n";
}

void ResponseBuilder::setStatusCode(int statusCode) {
	_statusCode = statusCode;
}

int ResponseBuilder::getStatusCode() {
	return _statusCode;
}

std::string ResponseBuilder::getStatusLine() {
	return _statusLine;
}

std::string ResponseBuilder::getContentType() {
	return _contentType;
}

size_t ResponseBuilder::getContentLength() {
	return _contentLength;
}

std::time_t ResponseBuilder::getLastModified() {
	return _lastModified;
}

std::string ResponseBuilder::getLastModifiedString() {
	std::stringstream ss;
	ss << _lastModified;
	return ss.str();
}

size_t ResponseBuilder::getDate() {
	return _date;
}

std::string ResponseBuilder::getDateString(){
	std::stringstream ss;
	ss << _date;
	return ss.str();
}