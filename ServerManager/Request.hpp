#pragma once
#include "../Includes.hpp"

class Request {
	public :
		Request();
		~Request();

		bool parseRequest(Config &config);

		//getters
		int getStatus() { return _status; };
		Method getMethod() { return _method; };
		bool isRedirect() { return _isRedirect; };
		bool isAutoIndex() { return _isAutoindex; };
		std::string getAutoIndexPath() {return _autoIndexPath; };
		std::string getPath() {return _path; };
		std::string getVersion() {return _version; };
		std::string getBody() {return _body; };
		std::map<std::string, std::string> getQueryParams() {return _queryParams; };
		std::map<std::string, std::string> getHeaders() {return _headers; };

	private:
		int _status;
		bool _isRedirect;
		bool _isAutoindex;
		Method _method;
		std::string _autoIndexPath;
		std::string _path;
		std::string _version;
		std::map<std::string, std::string> _queryParams;
		std::map<std::string, std::string> _headers;
		std::string _body;
};