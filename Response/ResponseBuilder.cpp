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
		if (resolver.isPathSafe(requestpath)) {
			
		}
	}
	
	// Implement logic to create the response based on the request and server configuration
	// This is a placeholder implementation and should be expanded based on actual requirements
	std::string response;
	return response; // Placeholder response
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