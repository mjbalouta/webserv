#pragma once
#include "Includes.hpp"

// HTTP Request parser class
// Parses raw HTTP/1.1 request text into structured fields
class Request {
public:

	// Default constructor for empty/invalid requests
	Request();
	// Construct and parse a request from raw HTTP data
	explicit Request(const std::string &raw);
	~Request();
	Request(const Request &src);
	Request &operator=(const Request &src);

	// Getters
	const std::string& getMethod() const;
	const std::string& getPath() const;
	const std::string& getVersion() const;
	const std::string& getBody() const;
	
	// Get a header value by key (case-insensitive). Returns empty string if not found.
	std::string getHeader(const std::string &key) const;
	
	// Check if a header exists (case-insensitive)
	bool hasHeader(const std::string &key) const;
	
	// Get all headers (keys are lowercased)
	const std::map<std::string, std::string>& getHeaders() const;
	
	// Check if the request appears to be valid
	bool isValid() const;
private:
	std::string _method;
	std::string _path;
	std::string _version;
	std::map<std::string, std::string> _headers; // keys are lowercased
	std::string _body;
	bool _valid;
	
	// Internal parsing helpers
	void parse(const std::string &raw);
	void parseRequestLine(const std::string &line);
	void parseHeaders(const std::string &headersBlock);
	void parseBody(const std::string &raw, size_t bodyStart);
};