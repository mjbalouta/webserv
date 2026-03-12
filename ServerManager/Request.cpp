#include "Request.hpp"

Request::Request()
	: _status(200), _isRedirect(false), _isAutoindex(false),
	  _method(GET), _autoIndexPath(""), _path(""), _version(""),
	  _host(""), _isChunked(false), _body("") {}

Request::~Request() {}

/**
 * @brief Converts raw HTTP text into normalized Request metadata.
 * @details
 *   Parses request line, headers, query parameters, host/chunked flags, and body.
 *   On validation failure, sets `_status` to the corresponding HTTP error code
 *   and returns false.
 * @param rawRequest Raw HTTP request bytes collected from socket reads.
 * @return true on successful parse; false on malformed/invalid input.
 */
bool Request::parseRequest(const std::string &rawRequest, size_t contentLength)
{
	// Reset request state before parsing a new message.
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

	// HTTP headers and body are separated by an empty line.
	size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return (printLog("❌ Malformed request: missing header terminator", RED), _status = 400, false);

	// Split raw payload into header block and body block.
	std::string head = rawRequest.substr(0, headerEnd);
	std::string body = rawRequest.substr(headerEnd + 4);

	// Read the request line: METHOD SP TARGET SP VERSION.
	std::istringstream headStream(head);
	std::string line;
	if (!std::getline(headStream, line))
		return (printLog("❌ Invalid request line", RED), _status = 400, false);

	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::istringstream requestLine(line);
	std::string methodToken;
	std::string target;
	if (!(requestLine >> methodToken >> target >> _version))
		return (printLog("❌ Malformed request line", RED), _status = 400, false);

	std::string trailingToken;
	if (requestLine >> trailingToken)
		return (printLog("❌ Invalid request line format", RED), _status = 400, false);

	// Map method token to internal enum.
	if (methodToken == "GET")
		_method = GET;
	else if (methodToken == "POST")
		_method = POST;
	else if (methodToken == "DELETE")
		_method = DELETE;
	else
		return (printLog("❌ Method not allowed", RED), _status = 405, false);

	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
		return (printLog("❌ Unsupported HTTP version", RED), _status = 505, false);

	// Split path and query string from request target.
	size_t queryPos = target.find('?');
	if (queryPos == std::string::npos)
		_path = target;
	else
	{
		_path = target.substr(0, queryPos);
		parseQueryString(target.substr(queryPos + 1), _queryParams);
	}

	if (_path.empty() || _path[0] != '/')
		return (printLog("❌ Invalid request target", RED), _status = 400, false);

	// Parse each header line as "Key: Value" and store normalized key/value pairs.
	while (std::getline(headStream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos || colonPos == 0)
			return (printLog("❌ Malformed header", RED), _status = 400, false);

		std::string rawKey = line.substr(0, colonPos);
		std::string rawValue = line.substr(colonPos + 1);
		std::string key = toLower(trimSpaces(rawKey));
		std::string value = trimSpaces(rawValue);
		if (key.empty())
			return (printLog("❌ Empty header key", RED), _status = 400, false);

		_headers[key] = value;
	}

	// Host header is mandatory in HTTP/1.1.
	if (_version == "HTTP/1.1" && _headers.find("host") == _headers.end())
		return (printLog("❌ Missing Host header", RED), _status = 400, false);

	// Cache frequently used metadata extracted from headers.
	std::map<std::string, std::string>::const_iterator hostIt = _headers.find("host");
	if (hostIt != _headers.end())
		_host = hostIt->second;

	std::map<std::string, std::string>::const_iterator transferEncodingIt = _headers.find("transfer-encoding");
	if (transferEncodingIt != _headers.end())
		_isChunked = (toLower(transferEncodingIt->second).find("chunked") != std::string::npos);

	// Parse and validate Content-Length when present.
	size_t expectedBodyLength = 0;
	std::map<std::string, std::string>::const_iterator contentLengthIt = _headers.find("content-length");

	// For POST in this implementation, Content-Length is required.
	//MISSING CHUNKED
	if (_method == POST && contentLengthIt == _headers.end())
		return (printLog("⚠️ Content-Length header missing", RED), _status = 411, false);

	// Copy body only when enough bytes are available.
	if (contentLengthIt != _headers.end())
	{
		expectedBodyLength = contentLength;
		if (body.size() < expectedBodyLength)
			return (printLog("❌ Incomplete request body", RED), _status = 400, false);
		_body = body.substr(0, expectedBodyLength);
	}
	else
		_body = "";

	// For POST in this implementation, Content-Type is required.
	if (_method == POST && _headers.find("content-type") == _headers.end())
		return (printLog("⚠️ Content-Type header missing", RED), _status = 400, false);

	// Request is syntactically valid and fully parsed.
	return true;
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