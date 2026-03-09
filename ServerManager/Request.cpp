#include "Request.hpp"

Request::Request()
	: _status(200),
	  _isRedirect(false),
	  _isAutoindex(false),
	  _method(GET),
	  _autoIndexPath(""),
	  _path(""),
	  _version(""),
	  _body("") {}

Request::~Request() {}

/**
 * @brief Parses the raw HTTP request data and populates the Request object.
 * @param config Active server configuration used for path resolution and validation.
 * @return true if parsing is successful and request is valid; false otherwise.
 */
bool Request::parseRequest(Config &config, const std::string request) {
	std::istringstream iss(request);
	std::string line;
	if (!std::getline(iss, line))
		return (printMessage("❌ Error reading request", RED), _status = 400, false);
	if (!parseRequestLine(line, config))
		return false;
	if (_isRedirect || _isAutoindex)
		return true;
	while (std::getline(iss, line) && !line.empty() && line != "\r")
	{
		trimSpaces(line);
		if (!parseHeaderLine(line))
			return false;
	}
	if (_method == POST)
	{
		if (!parseBody(iss, config))
			return false;
		std::string contentLength = getHeader("content-length");
		if (contentLength.empty())
			return (printMessage("⚠️ Content-Length header missing", RED), _status = 411, false);
		std::string contentType = getHeader("content-type");
		if (contentType.empty())
			return (printMessage("⚠️ Content-Type header missing", RED), _status = 400, false);
	}
	return true;
}
