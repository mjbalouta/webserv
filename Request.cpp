#include "Request.hpp"
#include "Exception.hpp"
#include "Utils.hpp"
#include <sstream>
#include <cctype>
#include <cstdlib>

Request::Request() : _valid(false) {}
Request::Request(const std::string &raw) : _valid(false) {
	parse(raw);
}
Request::~Request() {}
Request::Request(const Request &src)
	: _method(src._method)
	, _path(src._path)
	, _version(src._version)
	, _headers(src._headers)
	, _body(src._body)
	, _valid(src._valid) {}
Request &Request::operator=(const Request &src) {
	if (this != &src) {
		_method = src._method;
		_path = src._path;
		_version = src._version;
		_headers = src._headers;
		_body = src._body;
		_valid = src._valid;
	}
	return *this;
}


// Parse the raw HTTP request
void Request::parse(const std::string &raw) {
	std::istringstream stream(raw);
	std::string line;
	_valid = false;
	_method.clear();
	_path.clear();
	_version.clear();
	_headers.clear();
	_body.clear();

	// Parse request line (e.g., "GET / HTTP/1.1")
	if (!(std::getline(stream, line)))
		throw RequestException("Invalid request line 'e.g., GET / HTTP/1.1'");
	parseRequestLine(line);

	if (_method.empty() || _path.empty() || _version.empty())
		throw RequestException("Incomplete request line");
	// Parse headers until empty line
	while (std::getline(stream, line) && line != "\r") {
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);

			// Trim whitespace and normalize key to lowercase
			key = trim(key);
			key = toLower(key);
			value = trim(value);

			_headers[key] = value;
		}
	}
	// Parse body if Content-Length header exists
	std::map<std::string, std::string>::iterator it = _headers.find("content-length");
	if (it != _headers.end()) {
		int len = 0;
		if (!it->second.empty()) {
			len = stringToInt(it->second);
		}
		if (len > 0) {
			_body.resize(len);
			stream.read(&_body[0], len);
		}
	}
	// Mark as valid when request-line tokens exist
	_valid = true;
}

void Request::parseRequestLine(const std::string &line) {
	std::istringstream lineStream(line);
	if (!(lineStream >> _method >> _path >> _version)) {
		_method.clear();
		_path.clear();
		_version.clear();
		return;
	}
	if (!_version.empty() && _version[_version.size() - 1] == '\r')
		_version.erase(_version.size() - 1, 1);
}

// Getters
const std::string& Request::getMethod() const { return _method; }
const std::string& Request::getPath() const { return _path; }
const std::string& Request::getVersion() const { return _version; }
const std::string& Request::getBody() const { return _body; }

std::string Request::getHeader(const std::string &key) const {
	std::string lowerKey = toLower(key);
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerKey);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return _headers;
}

bool Request::hasHeader(const std::string &key) const {
	std::string lowerKey = toLower(key);
	return _headers.find(lowerKey) != _headers.end();
}

bool Request::isValid() const {
	return _valid;
}
