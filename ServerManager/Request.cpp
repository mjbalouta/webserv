#include "Request.hpp"
#include "../routing/ConfigResolved.hpp"
#include "../fileResourceManagement/FileSystemHandler.hpp"
#include "../fileResourceManagement/PathResolver.hpp"

/**
 * @brief Checks if a string is valid UTF-8 and contains only printable characters.
 * @param s Input string to validate.
 * @return true if valid, false otherwise.
 */
static bool isValidUtf8AndPrintable(const std::string &s) {
		size_t len = s.size();
		size_t i = 0;
		while (i < len) {
			unsigned char c = s[i];
			// Use std::isprint for ASCII, allow tab
			if (c < ASCII_MASK) {
				if (!std::isprint(c) && c != '\t') return false;
				i++;
				continue;
			}
			size_t remaining = len - i;
			if ((c & TWO_BYTE_MASK) == TWO_BYTE_PREFIX) {
				if (remaining < 2 || (static_cast<unsigned char>(s[i+1]) & CONTINUATION_MASK) != CONTINUATION_PREFIX)
					return false;
				i += 2;
			} else if ((c & THREE_BYTE_MASK) == THREE_BYTE_PREFIX) {
				if (remaining < 3 || (static_cast<unsigned char>(s[i+1]) & CONTINUATION_MASK) != CONTINUATION_PREFIX || (static_cast<unsigned char>(s[i+2]) & CONTINUATION_MASK) != CONTINUATION_PREFIX)
					return false;
				i += 3;
			} else if ((c & FOUR_BYTE_MASK) == FOUR_BYTE_PREFIX) {
				if (remaining < 4 || (static_cast<unsigned char>(s[i+1]) & CONTINUATION_MASK) != CONTINUATION_PREFIX || (static_cast<unsigned char>(s[i+2]) & CONTINUATION_MASK) != CONTINUATION_PREFIX || (static_cast<unsigned char>(s[i+3]) & CONTINUATION_MASK) != CONTINUATION_PREFIX)
					return false;
				i += 4;
			} else {
				return false;
			}
		}
		return true;
}

Request::Request()
	: _status(200), _isRedirect(false), _isAutoindex(false),
	  _method(GET), _autoIndexPath(""), _path(""), _version(""),
	  _host(""), _isChunked(false), _body("") {}

Request::~Request() {}

/**
 * @brief Resets all per-request fields before parsing a new HTTP message.
 */
void Request::resetStateForParsing()
{
	_status = 200;
	_isRedirect = false;
	_isAutoindex = false;
	_method = NONE;
	_autoIndexPath.clear();
	_path.clear();
	_version.clear();
	_host.clear();
	_isChunked = false;
	_queryParams.clear();
	_headers.clear();
	_body.clear();
}

/**
 * @brief Maps the raw HTTP method token to the internal enum.
 * @param methodToken Method string parsed from request-line.
 * @return true when method is supported, false otherwise.
 */
bool Request::parseMethodToken(const std::string &methodToken)
{
	if (methodToken == "GET")
		_method = GET;
	else if (methodToken == "POST")
		_method = POST;
	else if (methodToken == "DELETE")
		_method = DELETE;
	else
		return (printLog("🚨 Method not allowed", RED), _status = 405, false);

	return true;
}

/**
 * @brief Parses and validates the request-line (METHOD TARGET VERSION).
 * @param head Header block (without body), used to initialize line stream.
 * @param headStream Output stream positioned after request-line for header parsing.
 * @param target Output request target/path token.
 * @return true on success, false on malformed line or unsupported method/version.
 */
bool Request::parseRequestLine(const std::string &head, std::istringstream &headStream, std::string &target)
{
	headStream.clear();
	headStream.str(head);

	// The very first line of an HTTP request is the request-line:
	//   METHOD SP REQUEST-TARGET SP HTTP-VERSION CRLF
	std::string line;
	if (!std::getline(headStream, line))
		return (printLog("🚨 Invalid request line", RED), _status = 400, false);

	// std::getline keeps '\r' on Windows-style CRLF line endings — strip it.
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	// Enforce exactly one space between tokens in the request line
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);
	// There must be exactly two spaces, and no consecutive spaces
	if (firstSpace == std::string::npos || secondSpace == std::string::npos ||
		line.find(' ', secondSpace + 1) != std::string::npos ||
		secondSpace == firstSpace + 1)
		return (printLog("🚨 Invalid request line spacing", RED), _status = 400, false);

	// Split the request-line into its three whitespace-delimited tokens.
	std::istringstream requestLine(line);
	std::string methodToken;
	if (!(requestLine >> methodToken >> target >> _version))
		return (printLog("🚨 Malformed request line", RED), _status = 400, false);

	if (target.size() > 2048)
		return (printLog("🚨 URI too long", RED), _status = 414, false);
	// HTTP/1.x allows exactly three tokens on the request-line.
	// A fourth token means the client sent garbage.
	std::string trailingToken;
	if (requestLine >> trailingToken)
		return (printLog("🚨 Invalid request line format", RED), _status = 400, false);

	// Validate method token for whitespace or non-ASCII
	for (size_t i = 0; i < methodToken.size(); ++i) {
		unsigned char c = methodToken[i];
		if (std::isspace(static_cast<unsigned char>(c)) || c < 65 || c > 90) // 'A'-'Z'
			return (printLog("🚨 Invalid whitespace or non-uppercase in method", RED), _status = 400, false);
	}
	// Validate the method string and store the corresponding enum value.
	if (!parseMethodToken(methodToken))
		return false;

	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
		return (printLog("🚨 Unsupported HTTP version", RED), _status = 505, false);

	return true;
}

/**
 * @brief Splits request target into path and query map, then validates path.
 * @param target Raw request target token from request-line.
 * @return true on success, false when target/path is invalid.
 */
bool Request::parseTargetAndQuery(const std::string &target)
{
	size_t queryPos = target.find('?');
	if (queryPos == std::string::npos)
		_path = target;
	else
	{
		_path = target.substr(0, queryPos);
		parseQueryString(target.substr(queryPos + 1), _queryParams);
	}

	// An absolute-path request target must start with '/'
	if (_path.empty() || _path[0] != '/')
		return (printLog("🚨 Invalid request target", RED), _status = 400, false);

	return true;
}

/**
 * @brief Parses all HTTP header lines into normalized lowercase key/value pairs.
 * @param headStream Stream positioned at first header line.
 * @return true on success, false on malformed header syntax.
 */
bool Request::parseHeaders(std::istringstream &headStream)
{
	std::string line;
	int hostCount = 0;
	while (std::getline(headStream, line)) {
		// Strip the trailing '\r' left by CRLF line endings.
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		// Validate UTF-8 and printable characters in the header line
		if (!isValidUtf8AndPrintable(line))
			return (printLog("🚨 Invalid UTF-8 or non-printable in header", RED), _status = 400, false);

		// The first ':' separates the field name from the field value.
		// colonPos == 0 means the name is empty, which is invalid.
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos || colonPos == 0)
			return (printLog("🚨 Malformed header", RED), _status = 400, false);

		// Extract the raw key and raw value around the colon.
		std::string rawKey = line.substr(0, colonPos);
		std::string rawValue = line.substr(colonPos + 1);
		// Reject header values containing any tab character (edge test requirement)
		for (size_t i = 0; i < rawValue.size(); ++i) {
			if (rawValue[i] == '\t')
				return (printLog("🚨 Tab in header value", RED), _status = 400, false);
		}
/*			// Reject header keys containing any non-visible ASCII (only allow 33–126)
			for (size_t i = 0; i < rawKey.size(); ++i) {
				unsigned char c = rawKey[i];
				if (c < 33 || c > 126)
					return (printLog("🚨 Invalid character in header key", RED), _status = 400, false);
			}*/
		std::string key = toLower(trimSpaces(rawKey));
		std::string value = trimSpaces(rawValue);
		if (key.empty())
			return (printLog("🚨 Empty header key", RED), _status = 400, false);

		if (key == "host") {
			hostCount++;
			if (hostCount > 1)
				return (printLog("🚨 Multiple Host headers", RED), _status = 400, false);
		}
		_headers[key] = value;
	}

	return true;
}

/**
 * @brief Validates Host requirement for HTTP/1.1 and caches host value.
 * @return true on success, false when mandatory Host header is missing.
 */
bool Request::validateAndCacheHostHeader()
{
	// HTTP/1.1 clients MUST send a Host header
	// HTTP/1.0 clients are not required to
	if (_version == "HTTP/1.1") {
		if (_headers.find("host") == _headers.end())
			return (printLog("🚨 Missing Host header", RED), _status = 400, false);
	}

	// Cache the Host value in _host
	std::map<std::string, std::string>::const_iterator hostIt = _headers.find("host");
	if (hostIt != _headers.end())
		_host = hostIt->second;

	return true;
}

/**
 * @brief Caches Transfer-Encoding related parser flags.
 */
void Request::cacheTransferEncodingFlags()
{
	// Transfer-Encoding: chunked means the body arrives in size-prefixed chunks
	// We cache the flag here; actual chunk parsing is not yet implemented.
	std::map<std::string, std::string>::const_iterator transferEncodingIt = _headers.find("transfer-encoding");
	if (transferEncodingIt != _headers.end())
		_isChunked = (toLower(transferEncodingIt->second).find("chunked") != std::string::npos);
}

/**
 * @brief Validates body-related headers and extracts body bytes.
 * @param body Raw body bytes already received after header terminator.
 * @param contentLength Content length computed earlier by transport layer.
 * @return true on success, false on body/header validation failures.
 */
bool Request::parseAndValidateBody(const std::string &body, size_t contentLength)
{
	std::map<std::string, std::string>::const_iterator contentLengthIt = _headers.find("content-length");

	// A POST request must declare body length either via Content-Length,
	// or via Transfer-Encoding: chunked already decoded by transport.
	if (_method == POST && contentLengthIt == _headers.end() && !_isChunked)
		return (printLog("⚠️ Content-Length header missing", RED), _status = 404, false);

	if (_isChunked) {
/* 		std::string decoded;
		if (!decodeChunkedBody(body, decoded))
			return (printLog("🚨 Malformed chunked body", RED), _status = 400, false); 
		_body = decoded;*/
		_body = body;
	} else if (contentLengthIt != _headers.end()) {
		// If buffer is not enough bytes yet the request is incomplete.
		if (body.size() < contentLength)
			return (printLog("🚨 Incomplete request body", RED), _status = 400, false);
		// Copy exactly contentLength bytes to avoid reading into the next
		_body = body.substr(0, contentLength);
	} else {
		// No Content-Length header and non-POST method.
		_body = "";
	}

  //For both HTTP/1.0 and HTTP/1.1, Content-Type is recommended but not required for POST requests. Your server should accept POST requests without
	return true;
}

/**
 * @brief Converts raw HTTP text into normalized Request metadata.
 * @details
 *   Parses request line, headers, query parameters, host/chunked flags, and body.
 *   On validation failure, sets `_status` to the corresponding HTTP error code
 *   and returns false.
 * @param rawRequest Raw HTTP request bytes collected from socket reads.
 * @return true on successful parse; false on malformed/invalid input.
 */
bool Request::parseRequest(const std::string &rawRequest, size_t contentLength) {
	resetStateForParsing();

	// Every valid HTTP/1.x request contains a blank line (\r\n\r\n) that separates the header section from the optional message body.
	size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return (printLog("🚨 Malformed request: missing header terminator", RED), _status = 400, false);

	// Split at the blank line:
	//   head — everything before \r\n\r\n (request-line + headers)
	//   body — everything after  \r\n\r\n
	std::string head = rawRequest.substr(0, headerEnd);
	std::string body = rawRequest.substr(headerEnd + 4);
	std::istringstream headStream;
	std::string target;

	// Step 1 — Parse METHOD, TARGET, VERSION from the first line.
	if (!parseRequestLine(head, headStream, target))
		return false;
	// Step 2 — Split target into _path and _queryParams.
	if (!parseTargetAndQuery(target))
		return false;
	// Step 3 — Read all "Key: Value" header lines into the _headers map.
	if (!parseHeaders(headStream))
		return false;
	// Step 4 — Enforce Host requirement for HTTP/1.1 and cache _host.
	if (!validateAndCacheHostHeader())
		return false;
	// Step 5 — Set _isChunked if Transfer-Encoding: chunked is present.
	cacheTransferEncodingFlags();
	// Step 6 — Validate Content-Length / Content-Type and copy body into _body.
		bool bodyOk = parseAndValidateBody(body, contentLength);
		return bodyOk;
}

/**
 * @brief Returns a header value using case-insensitive lookup.
 * @param keyHeader Header name to find (e.g., "Host", "Content-Type").
 * @return std::string Header value if present, otherwise empty string.
 */
std::string Request::getHeader(const std::string &keyHeader) const
{
	std::string lowerKey = toLower(keyHeader);
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerKey);
	if (it != _headers.end())
		return it->second;
	return "";
}

/**
 * @brief Parses query string pairs separated by '&' into a map.
 * @param query Raw query string without leading '?'.
 * @param queryParams Output map populated with parsed pairs.
 */
void Request::parseQueryString(const std::string &query, std::map<std::string, std::string> &queryParams)
{
	if (query.empty())
		return;

	// Query strings have the form: key1=val1&key2=val2&flag
	// Split on '&' to get individual "key=value" (or bare "key") tokens.
	std::stringstream queryStream(query);
	std::string token;
	while (std::getline(queryStream, token, '&'))
	{
		if (token.empty())
			continue;
		size_t equalPos = token.find('=');
		if (equalPos == std::string::npos)
			queryParams[token] = "";
		else
			queryParams[token.substr(0, equalPos)] = token.substr(equalPos + 1);
	}
}

/**
 * @brief Returns the HTTP method as a string.
 * @return std::string One of: "GET", "POST", "DELETE", or "NONE".
 */
std::string Request::getMethodStr() const{
	switch (_method)
	{
		case GET:
			return "GET";
		case POST:
			return "POST";
		case DELETE:
			return "DELETE";
		case NONE:
		default:
			return "NONE";
	}
}

/**
 * @brief Needed to extract the specific query info from the request
 * 
 * @param name name of the query to search for
 * @return const std::string& 
 */
const std::string& Request::getSpecificQuery(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator it = _queryParams.find(name);
	if (it != _queryParams.end())
		return it->second; //returns the value for the <name> key
	
	static const std::string empty = "";
	return empty;
}

const std::string& Request::getCgiFullPath() const
{
	return _cgiFullPath;
}

const std::string& Request::getCgiInterpreter() const
{
	return _cgiInterpreter;
}

/**
 * @brief Parses request path, checks if it is safe (if it's not it sets the correct status for it)
 * and checks if it is a CGI request or not
 * 
 * @param routing 
 * @return true 
 * @return false 
 */
bool Request::isCgi(const ConfigResolved& routing)
{
	// const std::string& requestPath = getPath();
	_cgiFullPath = routing.getResolvedPath(*this);
	
	//checking if extension exists in the config file
	size_t dotPos = _cgiFullPath.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == _cgiFullPath.size() - 1)
		return false;
	std::string requestExtension = _cgiFullPath.substr(dotPos);
	const std::map<std::string, std::string>& cgiMap = routing.getCgi();
	std::map<std::string, std::string>::const_iterator it = cgiMap.find(requestExtension);
	if (it == cgiMap.end())
		return false;

	_cgiInterpreter = it->second;

	FileSystemHandler fs;
	PathResolver p;

	if (routing.getAlias().empty())
	{
		if (!p.isPathSafe(getPath(), routing.getRoot()))
		{
			setStatus(403);
			return false;
		}
	}
	else
	{
		if (!p.isPathSafe(getPath(), routing.getAlias()))
		{
			setStatus(403);
			return false;
		}
	}
	// For CGI, we don't need the file to exist - the CGI interpreter handles the request.
	// Only check that it's not a directory (we can't execute a directory as CGI).
	if (fs.isDirectory(_cgiFullPath))
	{
		setStatus(403);
		return false;
	}
		
	return true;
}