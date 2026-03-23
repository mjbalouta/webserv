#include "ResponseBuilder.hpp"

#include <cerrno>

/**
 * @brief Formats a time value as an HTTP date string.
 * 
 * @param t The time value to format.
 * @return std::string The formatted date string.
 */
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

/**
 * @brief Checks if the given URI path starts with the specified location path, ensuring that it matches a location boundary.
 * 
 * @param uriPath The URI path to check.
 * @param locPath The location path to compare against.
 * @return true if the URI path starts with the location path and matches a boundary, false otherwise.
 */
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
	if (locPath[locPath.size() - 1] == '/')
		return true;
	return (uriPath[locPath.size()] == '/');
}

/**
 * @brief Ensures that the given path starts with a leading slash.
 * 		  If the path is empty, it returns "/". If the path already starts with a slash, it returns the path unchanged.
 * 		  Otherwise, it prepends a slash to the path and returns the modified string.
 * 
 * @param p The path to check.
 * @return std::string The path with a leading slash.
 */
std::string ResponseBuilder::ensureLeadingSlash(const std::string &p)
{
	if (p.empty())
		return "/";
	if (p[0] == '/')
		return p;
	return "/" + p;
}

/**
 * @brief Ensures that the given path ends with a trailing slash.
 * 		  If the path is empty or already ends with a slash, it returns the path unchanged. 
 * 		  Otherwise, it appends a slash to the path and returns the modified string.
 * 
 * @param p The path to check.
 * @return std::string The path with a trailing slash.
 */
std::string ResponseBuilder::ensureTrailingSlash(const std::string &p)
{
	if (!p.empty() && p[p.size() - 1] == '/')
		return p;
	return p + "/";
}


/**
 * @brief Joins two path components into a single path.
 * 
 * @param a The first path component.
 * @param b The second path component.
 * @return std::string The joined path.
 */
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


/**
 * @brief Builds an autoindex HTML body for a directory listing based on the given URI path and filesystem path.
 * 
 * @param uriPath 
 * @param dirFsPath 
 * @param fs 
 * @return std::string 
 */
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


/**
 * @brief Checks if the given HTTP method is allowed for the current location.
 * 
 * @param method The HTTP method to check.
 * @param resolvedConfig The resolved configuration for the current location.
 * @return true if the method is allowed, false otherwise.
 */
bool ResponseBuilder::isMethodAllowed(const std::string &method, const ConfigResolved &resolvedConfig)
{
	const std::vector<std::string> &locMethods = resolvedConfig.getAllowedMethods();
	if (!locMethods.empty())
		for (size_t i = 0; i < locMethods.size(); ++i)
			if (locMethods[i] == method)
				return true;
	return false;
}

/**
 * @brief Returns the appropriate response for the given request and configuration.
 * 
 * @param request The incoming HTTP request.
 * @param resolvedConfig The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::returnResponse(const Request& request, const ConfigResolved& resolvedConfig, bool keepAlive) {
	// Reset state (ResponseBuilder may be reused across requests).
	_statusCode = 0;
	_statusLine.clear();
	_contentType.clear();
	_contentLength = 0;
	_date = 0;
	_lastModified = 0;
	_body.clear();
	_location.clear();
	_keepAlive = false;

	// Transport/connection policy is decided by the server loop; ResponseBuilder
	// just reflects it in the Connection header.
	_keepAlive = keepAlive;

	if (request.getStatus() != 200)
		return returnGenericErrorResponse(request.getStatus(), request, resolvedConfig);

	if (!isMethodAllowed(request.getMethodStr(), resolvedConfig))
		return returnGenericErrorResponse(405, request, resolvedConfig);

	// 3) Handle explicit "return" directive (redirect/custom response).
	if (resolvedConfig.getReturnStatusCode())	
	{
		return buildRedirectResponse(request, resolvedConfig);
	} // General idea of redirect response
	// Resolve target path using the merged config (server root or location alias/root).

	std::string fileSystemPath = resolvedConfig.getResolvedPath(request);
	if (fileSystemPath.empty())
		return returnGenericErrorResponse(404, request, resolvedConfig);

 	std::string base;
	if (resolvedConfig.getAlias().empty())
		base = resolvedConfig.getRoot();
	else
		base = resolvedConfig.getAlias();
	if (!base.empty())
	{
		std::string rel = request.getPath();
		if (!resolvedConfig.getAlias().empty())
		{
			const std::string location = resolvedConfig.getLocationPath();
			if (!location.empty() && rel.find(location) == 0)
				rel.erase(0, location.size());
		}
		if (!rel.empty() && rel[0] == '/')
			rel.erase(0, 1);
		if (!pathResolver.isPathSafe(rel, base))
			return returnGenericErrorResponse(403, request, resolvedConfig);
	}
	else
	{
		// With no base directory configured, conservative approach about traversal attempts.
		if (request.getPath().find("..") != std::string::npos)
			return returnGenericErrorResponse(403, request, resolvedConfig);
	} 
	
	fileSystemPath = pathResolver.normalizePath(fileSystemPath);

	if (request.getMethodStr() == "POST")
		return buildPostResponse(request, resolvedConfig);
	if (request.getMethodStr() == "DELETE")
		return buildDeleteResponse(request, fileSystemPath, resolvedConfig);

	if (fileSystemHandler.pathExists(fileSystemPath) && fileSystemHandler.isDirectory(fileSystemPath))
	{
		// nginx-like: if URI doesn't end with '/', redirect to add it.
		if (!request.getPath().empty() && request.getPath()[request.getPath().size() - 1] != '/')
		{
			_statusCode = 301;
			_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
			_location = request.getPath() + "/";
			_contentType = "text/html";
			_body.clear();
			_contentLength = 0;

			std::string response = _statusLine;
			response += "Content-Type: " + _contentType + "\r\n";
			response += "Content-Length: " + getContentLengthString() + "\r\n";
			response += "Location: " + _location + "\r\n";
			response += "Date: " + formatHttpDate(std::time(NULL)) + "\r\n";
			response += std::string("Connection: ") + (_keepAlive ? "keep-alive" : "close") + "\r\n\r\n";
			return response;
		}

		std::string uriWithSlash = ensureTrailingSlash(request.getPath());
		return buildDirectoryListingResponse(request, uriWithSlash, fileSystemPath, resolvedConfig);
	}
	else if (fileSystemHandler.pathExists(fileSystemPath) && fileSystemHandler.isReadable(fileSystemPath))
		return buildFileResponse(request, fileSystemPath, resolvedConfig);
	else if (fileSystemHandler.pathExists(fileSystemPath) && !fileSystemHandler.isReadable(fileSystemPath))
		return returnGenericErrorResponse(403, request, resolvedConfig);
	else
		return returnGenericErrorResponse(404, request, resolvedConfig);
}

std::string ResponseBuilder::buildPostResponse(const Request& request, const ConfigResolved& resolvedConfig)
{
	std::string uploadStore = resolvedConfig.getUploadStore();
	if (uploadStore.empty())
		return returnGenericErrorResponse(501, request, resolvedConfig);

	if (!fileSystemHandler.pathExists(uploadStore) || !fileSystemHandler.isDirectory(uploadStore))
		return returnGenericErrorResponse(500, request, resolvedConfig);
	if (!fileSystemHandler.isWritable(uploadStore))
		return returnGenericErrorResponse(403, request, resolvedConfig);

	std::string locationPath = resolvedConfig.getLocationPath();
	std::string rest = request.getPath();
	if (!locationPath.empty() && startsWithLocationBoundary(rest, locationPath))
		rest.erase(0, locationPath.size());
	if (!rest.empty() && rest[0] == '/')
		rest.erase(0, 1);

	if (rest.empty())
		return returnGenericErrorResponse(400, request, resolvedConfig);
	if (rest.find('/') != std::string::npos || rest.find("..") != std::string::npos)
		return returnGenericErrorResponse(400, request, resolvedConfig);
	if (!pathResolver.isPathSafe(rest, uploadStore))
		return returnGenericErrorResponse(403, request, resolvedConfig);

	std::string targetPath = joinPathSimple(uploadStore, rest);
	targetPath = pathResolver.normalizePath(targetPath);
	bool existed = fileSystemHandler.pathExists(targetPath);

	if (!fileSystemHandler.writeFile(targetPath, request.getBody()))
		return returnGenericErrorResponse(500, request, resolvedConfig);

	if (existed)
	{
		_statusCode = 204;
		_contentType = "text/plain";
		_body.clear();
		_contentLength = 0;
	}
	else
	{
		_statusCode = 201;
		_contentType = "text/plain";
		_body = "Created\n";
		_contentLength = _body.size();
		_location = request.getPath();
	}

	_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	std::string response = _statusLine;
	setStandardHeaders(response, _contentType);
	if (_statusCode == 201 && !_location.empty())
		response += "Location: " + _location + "\r\n";
	response += "\r\n" + _body;
	return response;
}

std::string ResponseBuilder::buildDeleteResponse(const Request& request, const std::string& fileSystemPath, const ConfigResolved& resolvedConfig)
{
	std::string targetPath = fileSystemPath;

	std::string uploadStore = resolvedConfig.getUploadStore();
	if (!uploadStore.empty())
	{
		std::string locationPath = resolvedConfig.getLocationPath();
		std::string rest = request.getPath();
		if (!locationPath.empty() && startsWithLocationBoundary(rest, locationPath))
			rest.erase(0, locationPath.size());
		if (!rest.empty() && rest[0] == '/')
			rest.erase(0, 1);
		if (rest.empty())
			return returnGenericErrorResponse(400, request, resolvedConfig);
		if (rest.find('/') != std::string::npos || rest.find("..") != std::string::npos)
			return returnGenericErrorResponse(400, request, resolvedConfig);
		if (!pathResolver.isPathSafe(rest, uploadStore))
			return returnGenericErrorResponse(403, request, resolvedConfig);
		targetPath = joinPathSimple(uploadStore, rest);
		targetPath = pathResolver.normalizePath(targetPath);
	}

	if (!fileSystemHandler.pathExists(targetPath))
		return returnGenericErrorResponse(404, request, resolvedConfig);
	if (fileSystemHandler.isDirectory(targetPath))
		return returnGenericErrorResponse(403, request, resolvedConfig);

	errno = 0; // Check if using errno is not illegal in this context
	if (!fileSystemHandler.removeFile(targetPath))
	{
		if (errno == EACCES || errno == EPERM)
			return returnGenericErrorResponse(403, request, resolvedConfig);
		return returnGenericErrorResponse(500, request, resolvedConfig);
	}

	_statusCode = 204;
	_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	_contentType = "text/plain";
	_body.clear();
	_contentLength = 0;
	std::string response = _statusLine;
	setStandardHeaders(response, _contentType);
	response += "\r\n";
	return response;
}

/**
 * @brief Returns a redirect error response for the given status code and request.
 * 
 * @param statusCode The HTTP status code for the error.
 * @param request The incoming HTTP request.
 * @param matchedLocation The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::returnRedirectErrorResponse(int statusCode, const Request& request, const ConfigResolved& matchedLocation){
		_statusCode = statusCode;
		_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
		_contentType = "text/html";
		if (matchedLocation.getLocationPath().empty() || matchedLocation.getReturnMessage().empty()){
			_body = "";
			_contentLength = 0;
		}
		else{
			_body = matchedLocation.getReturnMessage();
			_contentLength = _body.size();
		}
		std::string response = _statusLine;
		response += "Content-Type: " + _contentType + "\r\n";
		response += "Content-Length: " + getContentLengthString() + "\r\n";
		response += "Date: " + formatHttpDate(std::time(NULL)) + "\r\n";
		response += "Last-Modified: " + formatHttpDate(std::time(NULL)) + "\r\n";
		response += std::string("Connection: ") + (_keepAlive ? "keep-alive" : "close") + "\r\n";
		response += "\r\n";
		if (request.getMethodStr() != "HEAD")
			response += _body;
		return response;
}

/**
 * @brief Returns a generic error response for the given status code and request.
 * 
 * @param statusCode The HTTP status code for the error.
 * @param request The incoming HTTP request.
 * @param config The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::returnGenericErrorResponse(int statusCode, const Request& request, const ConfigResolved& config){
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
		response += "\r\n";
		if (request.getMethodStr() != "HEAD")
			response += _body;
		return response;
}


/**
 * @brief Sets the standard headers for the HTTP response.
 * 
 * @param response The HTTP response string.
 * @param contentType The content type for the response.
 */
void ResponseBuilder::setStandardHeaders(std::string& response, const std::string& contentType) {
	response += "Content-Type: " + contentType + "\r\n";
	response += "Content-Length: " + getContentLengthString() + "\r\n";
	if (_date == 0)
		_date = static_cast<size_t>(std::time(NULL));
	if (_lastModified == 0)
		_lastModified = static_cast<std::time_t>(_date);
	response += "Date: " + formatHttpDate(static_cast<std::time_t>(_date)) + "\r\n";
	response += "Last-Modified: " + formatHttpDate(_lastModified) + "\r\n";
	response += std::string("Connection: ") + (_keepAlive ? "keep-alive" : "close") + "\r\n";
}

/**
 * @brief Builds a redirect response based on the given request and matched configuration.
 * 
 * @param request The incoming HTTP request.
 * @param matched The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::buildRedirectResponse(const Request& request, const ConfigResolved& matched) {
	int code = matched.getReturnStatusCode();
	if (code >= 400)
		return returnGenericErrorResponse(code, request, matched);
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
	response += std::string("Connection: ") + (_keepAlive ? "keep-alive" : "close") + "\r\n\r\n";
	if (request.getMethodStr() != "HEAD")
		response += _body;
	return response;
}

/**
 * @brief Builds a file response based on the given request, file path, and configuration.
 * 
 * @param request The incoming HTTP request.
 * @param filePath The path to the file to serve.
 * @param config The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::buildFileResponse(const Request& request, const std::string& filePath, const ConfigResolved& config){
	_statusCode = 200;
	_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	_contentType = mimeTypeResolver.getTypeByExtension(filePath);
	_contentLength = fileSystemHandler.getFileSize(filePath);
	_lastModified = fileSystemHandler.getLastMODTime(filePath);
	try{
		_body = fileSystemHandler.readFile(filePath, config.getMaxBodySize());
	}
	catch (const std::exception& e){
		(void)e;
		return returnGenericErrorResponse(500, request, config);
	}
	std::string response = _statusLine;
	setStandardHeaders(response, _contentType);
	response += "\r\n";
	if (request.getMethodStr() != "HEAD")
		response += _body;
	return response;
}

/**
 * @brief Builds a directory listing response based on the given request and configuration.
 * 
 * @param request The incoming HTTP request.
 * @param uriPath The URI path for the directory.
 * @param dirFsPath The file system path for the directory.
 * @param resolvedConfig The resolved configuration for the current location.
 * @return std::string The HTTP response.
 */
std::string ResponseBuilder::buildDirectoryListingResponse(const Request& request, const std::string& uriPath, const std::string& dirFsPath, const ConfigResolved& resolvedConfig) {
	_statusCode = 200;
	_statusLine = request.getVersion() + " " + getStatusCodeString() + " " + error.getReasonPhrase(_statusCode) + "\r\n";
	_contentType = "text/html";
	_body.clear();

	// Serve the first configured index file that exists.
	const std::vector<std::string>& indexes = resolvedConfig.getIndexes();
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		const std::string& indexName = indexes[i];
		if (indexName.empty())
			continue;
		std::string indexFsPath = joinPathSimple(dirFsPath, indexName);
		if (fileSystemHandler.pathExists(indexFsPath) && !fileSystemHandler.isDirectory(indexFsPath) && fileSystemHandler.isReadable(indexFsPath))
			return buildFileResponse(request, indexFsPath, resolvedConfig);
	}

	if (resolvedConfig.getAutoIndex())
	{
		_body = buildAutoIndexBody(uriPath, dirFsPath, fileSystemHandler);
		_contentLength = _body.size();
		std::string response = _statusLine;
		setStandardHeaders(response, _contentType);
		response += "\r\n";
		if (request.getMethodStr() != "HEAD")
			response += _body;
		return response;
	}

	return returnGenericErrorResponse(403, request, resolvedConfig);
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