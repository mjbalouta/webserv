#pragma once

#include "../Includes.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../ServerManager/Request.hpp"
#include "../fileResourceManagement/PathResolver.hpp"
#include "../fileResourceManagement/FileSystemHandler.hpp"
#include "../fileResourceManagement/MimeTypeResolver.hpp"
#include "../fileResourceManagement/ErrorPageGenerator.hpp"
#include "../routing/ConfigResolved.hpp"

class ResponseBuilder{
	public:
		std::string returnResponse(const Request& request, const ConfigResolved& resolvedConfig, bool keepAlive);
	private:
		int _statusCode;
		std::string _statusLine;
		std::string _contentType;
		size_t _contentLength;
		size_t _date;
		std::time_t _lastModified;
		std::string _body;
		std::string _location;
		bool _keepAlive;

		ErrorPageGenerator error;
		PathResolver pathResolver;
		FileSystemHandler fileSystemHandler;
		MimeTypeResolver mimeTypeResolver;

		int getStatusCode();
		int determineStatusCode(const std::string& request, const ConfigResolved& resolvedConfig);

		 std::string returnGenericErrorResponse(int statusCode, const Request& request, const ConfigResolved& resolvedConfig);
		 std::string returnRedirectErrorResponse(int statusCode, const Request& request, const ConfigResolved& resolvedConfig);

		std::string getStatusCodeString();
		std::string getStatusLine();
		std::string getContentType();
		size_t getContentLength();
		std::string getContentLengthString();
		std::time_t getLastModified();
		size_t getDate();
		std::string getDateString();
		std::string getLastModifiedString();

		void setStatusLine(int statusCode);
		void setContentType(const std::string& filePath);
		void setContentLength(size_t contentLength);
		void setDate(std::time_t date);
		void setLastModified(std::time_t lastModified);
		void setBody(const std::string& body);

		bool isMethodAllowed(const std::string &method, const ConfigResolved &resolvedConfig);
		std::string buildAutoIndexBody(const std::string &uriPath, const std::string &dirFsPath, FileSystemHandler &fs);
		std::string joinPathSimple(const std::string &a, const std::string &b);
		std::string ensureTrailingSlash(const std::string &p);
		std::string ensureLeadingSlash(const std::string &p);
		bool startsWithLocationBoundary(const std::string &uriPath, const std::string &locPath);
		std::string formatHttpDate(std::time_t t);

		 std::string buildRedirectResponse(const Request &request, const ConfigResolved &matched);
		 std::string buildFileResponse(const Request& request, const std::string& filePath, const ConfigResolved& resolvedConfig);
		 std::string buildErrorResponse(int statusCode, const ConfigResolved& resolvedConfig);
		std::string buildDirectoryListingResponse(const Request& request, const std::string& uriPath, const std::string& dirFsPath, const ConfigResolved& resolvedConfig);
		//std::string buildCGIResponse(const std::string& scriptPath, const ConfigResolved& resolvedConfig);
		void setStandardHeaders(std::string& response, const std::string& contentType);
		void replaceTag(std::string &content, const std::string &tag, const std::string &value);
		void insertServerInfo(const ConfigResolved& config);	
};