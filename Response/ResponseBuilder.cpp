#include "ResponseBuilder.hpp"


std::string ResponseBuilder::formatHttpDate(std::time_t t)
{
	// RFC 7231 IMF-fixdate: Sun, 06 Nov 1994 08:49:37 GMT
	char buf[64];
	std::tm *gmt = std::gmtime(&t);
	if (!gmt)
		return "";
	if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt) == 0)
		return "";
	return std::string(buf);
}

bool ResponseBuilder::startsWithLocationBoundary(const std::string &uriPath, const std::string &locPath)
{
	if (locPath.empty())
		return false;
	if (uriPath.size() < locPath.size())
		return false;
	if (uriPath.compare(0, locPath.size(), locPath) != 0)
		return false;
	if (uriPath.size() == locPath.size())
		return true;
	// Boundary: either location ends with '/', or next char in uri is '/'
	if (locPath[locPath.size() - 1] == '/')
		return true;
	return (uriPath[locPath.size()] == '/');
}

std::string ResponseBuilder::ensureLeadingSlash(const std::string &p)
{
	if (p.empty())
		return "/";
	if (p[0] == '/')
		return p;
	return "/" + p;
}

std::string ResponseBuilder::ensureTrailingSlash(const std::string &p)
{
	if (!p.empty() && p[p.size() - 1] == '/')
		return p;
	return p + "/";
}

std::string ResponseBuilder::joinPathSimple(const std::string &a, const std::string &b)
{
	if (a.empty())
		return b;
	if (b.empty())
		return a;
	if (a[a.size() - 1] == '/' && b[0] == '/')
		return a + b.substr(1);
	if (a[a.size() - 1] != '/' && b[0] != '/')
		return a + "/" + b;
	return a + b;
}

std::string ResponseBuilder::buildAutoIndexBody(const std::string &uriPath, const std::string &dirFsPath, FileSystemHandler &fs)
{
	std::vector<std::string> entries = fs.listDirectory(dirFsPath);
	std::string body;
	body += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
	body += "<title>Index of " + uriPath + "</title></head><body>";
	body += "<h1>Index of " + uriPath + "</h1><ul>";
	for (size_t i = 0; i < entries.size(); ++i)
	{
		std::string name = entries[i];
		std::string entryFs = joinPathSimple(dirFsPath, name);
		bool isDir = fs.isDirectory(entryFs);
		std::string href = ensureTrailingSlash(uriPath) + name + (isDir ? "/" : "");
		body += "<li><a href=\"" + href + "\">" + name + (isDir ? "/" : "") + "</a></li>";
	}
	body += "</ul></body></html>";
	return body;
}

bool ResponseBuilder::isMethodAllowed(const std::string &method, const LocationConfig *loc, const ServerConfig &config)
{
	if (loc)
	{
		const std::vector<std::string> &locMethods = loc->getAllowedMethods();
		if (!locMethods.empty())
		{
			for (size_t i = 0; i < locMethods.size(); ++i)
				if (locMethods[i] == method)
					return true;
			return false;
		}
	}
	const std::vector<std::string> &srvMethods = config.getAllowedMethods();
	if (srvMethods.empty())
		return true;
	for (size_t i = 0; i < srvMethods.size(); ++i)
		if (srvMethods[i] == method)
			return true;
	return false;
}


std::string ResponseBuilder::returnResponse(const Request& request, const ServerConfig& config) {
	if (request.getStatus() != 200)
		return returnGenericErrorResponse(request.getStatus(), request, config);
	
	std::string uriPath = request.getPath();

	const LocationConfig *matchedLocation = NULL;
	const std::vector<LocationConfig> &locations = config.getLocations();
	for (size_t i = 0; i < locations.size(); ++i)
	{
		if (!startsWithLocationBoundary(uriPath, locations[i].getPath()))
			continue;
		if (!matchedLocation || matchedLocation->getPath().size() < locations[i].getPath().size())
			matchedLocation = &locations[i];
	}
	if (!matchedLocation)
		return returnGenericErrorResponse(404, request, config);

	// 2) Enforce method allow-list (location overrides server when set).
	if (!isMethodAllowed(request.getMethodStr(), matchedLocation, config))
		return returnGenericErrorResponse(405, request, config);

	// 3) Handle explicit "return" directive (redirect/custom response).
	if (matchedLocation->getReturnStatusCode())	
	{
/* 		int code = matchedLocation->getReturnStatusCode();
		if (code >= 400)
			return returnGenericErrorResponse(code, request, config);
		_statusCode = code;
		std::stringstream ss;
		ss << _statusCode;
		_statusLine = request.getVersion() + " " + ss.str() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_location = matchedLocation->getReturnURL();
		_contentType = "text/html";
		if (matchedLocation->getReturnMessage().empty() || code == 304 || code == 204)
		{
			_body.clear();
			_contentLength = 0;
		}
		else
		{
			_body = matchedLocation->getReturnMessage();
			_contentLength = _body.size();
		}

		std::string response = _statusLine;
		response += "Content-Type: " + _contentType + "\r\n";
		response += "Content-Length: " + getContentLengthString() + "\r\n";
		if (!_location.empty())
			response += "Location: " + _location + "\r\n";
		response += "Date: " + formatHttpDate(std::time(NULL)) + "\r\n";
		response += "Connection: close\r\n\r\n";
		response += _body;
		return response; */
		return buildRedirectResponse(*matchedLocation, request, config);
	} // General idea of redirect response
	if (matchedLocation->getAutoIndex())
	{
		std::string dirFSPath = matchedLocation->getRoot() + uriPath.substr(matchedLocation->getPath().size());
		std::string body = buildAutoIndexBody(uriPath, dirFSPath, fileSystemHandler);
		_statusCode = 200;
		std::stringstream ss;
		ss << _statusCode;
		_statusLine = request.getVersion() + " " + ss.str() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_contentType = "text/html";
		_body = body;
		_contentLength = _body.size();
		std::string response = _statusLine;
		setStandardHeaders(response, _contentType);
		response += "\r\n" + _body;
		return response;
	}
	if (matchedLocation->getAliasFlag() == true)
		uriPath = matchedLocation->getAlias() + uriPath.substr(matchedLocation->getPath().size());
	std::string fileSystemPath;
	if (matchedLocation->getRoot().empty())
		fileSystemPath = pathResolver.resolveToFilesystem(uriPath.substr(matchedLocation->getPath().size()), config.getRoot());
	else
		fileSystemPath = pathResolver.resolveToFilesystem(uriPath.substr(matchedLocation->getPath().size()), matchedLocation->getRoot());
	if (fileSystemPath.empty())
		return returnGenericErrorResponse(404, request, config);
	else if (fileSystemPath == "path is not safe")
		return returnGenericErrorResponse(403, request, config);

	if (fileSystemHandler.pathExists(fileSystemPath) && fileSystemHandler.isReadable(fileSystemPath))
		return buildFileResponse(fileSystemPath, config);
	else if (fileSystemHandler.pathExists(fileSystemPath) && fileSystemHandler.isDirectory(fileSystemPath))
	{
		if (fileSystemPath[fileSystemPath.size() - 1] != '/')
			fileSystemPath += '/';
//		return buildDirectoryListingResponse(fileSystemPath, config);
		return "";
	}
	else if (fileSystemHandler.pathExists(fileSystemPath) && !fileSystemHandler.isReadable(fileSystemPath))
		return returnGenericErrorResponse(403, request, config);
	else
		return returnGenericErrorResponse(404, request, config);
	
//	return response; // Placeholder response
}

std::string ResponseBuilder::returnRedirectErrorResponse(int statusCode, const Request& request, const LocationConfig* matchedLocation){
		_statusCode = statusCode;
		_statusLine = request.getVersion() + " " + getStatusCodeString(); + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_contentType = "text/html";
		if (!matchedLocation || matchedLocation->getReturnMessage().empty()){
			_body = "";
			_contentLength = 0;
		}
		else{
			_body = matchedLocation->getReturnMessage();
			_contentLength = _body.size();
		}
		std::string response = _statusLine;
		response += "Content-Type: " + _contentType + "\r\n";
		response += "Content-Length: " + getContentLengthString() + "\r\n";
		response += "Date: " + formatHttpDate(std::time(NULL)) + "\r\n";
		response += "Last-Modified: " + formatHttpDate(std::time(NULL)) + "\r\n";
		response += "Connection: close\r\n";
		response += "\r\n" + _body;
		return response;
}


std::string ResponseBuilder::returnGenericErrorResponse(int statusCode, const Request& request, const ServerConfig& config){
		_statusCode = statusCode;
		_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_contentType = "text/html";
		std::string errorPage = error.loadCustomErrorPage(_statusCode, config);
		if (errorPage.empty())
			errorPage = error.generateErrorPage(_statusCode, error.getReasonPhrase(_statusCode));
		_body = errorPage;
		_contentLength = _body.size();
		std::string response = _statusLine;
		setStandardHeaders(response, _contentType);
		response += "\r\n" + _body;
		return response;
}

void ResponseBuilder::setStandardHeaders(std::string& response, const std::string& contentType) {
	response += "Content-Type: " + contentType + "\r\n";
	response += "Content-Length: " + getContentLengthString() + "\r\n";
	if (_date == 0)
		_date = static_cast<size_t>(std::time(NULL));
	if (_lastModified == 0)
		_lastModified = static_cast<std::time_t>(_date);
	response += "Date: " + formatHttpDate(static_cast<std::time_t>(_date)) + "\r\n";
	response += "Last-Modified: " + formatHttpDate(_lastModified) + "\r\n";
	response += "Connection: close\r\n";
}

std::string ResponseBuilder::buildRedirectResponse(const LocationConfig& matched, const Request& request, const ServerConfig& config){
	int code = matched.getReturnStatusCode();
	if (code >= 400)
		return returnGenericErrorResponse(code, request, config);
	_statusCode = code;

	_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	_location = matched.getReturnURL();
	_contentType = "text/html";
	if (matched.getReturnMessage().empty() || code == 304 || code == 204)
	{
		_body.clear();
		_contentLength = 0;
	}
	else
	{
		_body = matched.getReturnMessage();
		_contentLength = _body.size();
	}

	std::string response = _statusLine;
	response += "Content-Type: " + _contentType + "\r\n";
	response += "Content-Length: " + getContentLengthString() + "\r\n";
	if (!_location.empty())
		response += "Location: " + _location + "\r\n";
	response += "Date: " + formatHttpDate(std::time(NULL)) + "\r\n";
	response += "Connection: close\r\n\r\n";
	response += _body;
	return response;
}

std::string ResponseBuilder::buildFileResponse(const std::string& filePath, const ServerConfig& config){
	_statusCode = 200;
	_statusLine = "HTTP/1.1 " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	_contentType = mimeTypeResolver.getTypeByExtension(filePath);
	_contentLength = fileSystemHandler.getFileSize(filePath);
	_lastModified = fileSystemHandler.getLastMODTime(filePath);
	_body = fileSystemHandler.readFile(filePath, config.getMaxBodySize());
	std::string response = _statusLine;
	setStandardHeaders(response, _contentType);
	response += "\r\n" + _body;
	return response;
}

int ResponseBuilder::getStatusCode() {
	return _statusCode;
}

std::string ResponseBuilder::getStatusCodeString() {
	std::stringstream ss;
	ss << _statusCode;
	return ss.str();
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

std::string ResponseBuilder::getContentLengthString() {
	std::stringstream ss;
	ss << _contentLength;
	return ss.str();
}

std::time_t ResponseBuilder::getLastModified() {
	return _lastModified;
}

std::string ResponseBuilder::getLastModifiedString() {
	return formatHttpDate(_lastModified);
}

size_t ResponseBuilder::getDate() {
	return _date;
}

std::string ResponseBuilder::getDateString(){
	return formatHttpDate(static_cast<std::time_t>(_date));
}