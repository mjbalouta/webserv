#pragma once
#include "../Includes.hpp"

enum Method {
	GET,
	POST,
	DELETE,
	NONE
};

class Request {
	public :
		Request();
		~Request();

		bool parseRequest(const std::string &rawRequest, size_t knownContentLength);
		void parseQueryString(const std::string &query, std::map<std::string, std::string> &queryParams);

		//getters
		int getStatus() const { return _status; };
		Method getMethod() const { return _method; };
		bool isRedirect() const { return _isRedirect; };
		bool isAutoIndex() const { return _isAutoindex; };
		const std::string &getAutoIndexPath() const { return _autoIndexPath; };
		const std::string &getPath() const { return _path; };
		const std::string &getVersion() const { return _version; };
		const std::string &getBody() const { return _body; };
		const std::string &getHost() const { return _host; };
		bool isChunked() const { return _isChunked; };
		const std::map<std::string, std::string> &getQueryParams() const { return _queryParams; };
		const std::map<std::string, std::string> &getHeaders() const { return _headers; };
		std::string getHeader(const std::string &keyHeader) const;

	private:
		int _status;
		bool _isRedirect;
		bool _isAutoindex;
		Method _method;
		std::string _autoIndexPath;
		std::string _path;
		std::string _version;
		std::string _host;
		bool _isChunked;
		std::map<std::string, std::string> _queryParams;
		std::map<std::string, std::string> _headers;
		std::string _body;
};