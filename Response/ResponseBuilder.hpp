#pragma once

#include "../Includes.hpp"

class ResponseBuilder{
	public:
		std::string returnResponse(const Request& request, const ServerConfig& config);
	private:
		int _statusCode;
		std::string _statusLine;
		std::string _contentType;
		size_t _contentLength;
		size_t _date;
		std::time_t _lastModified;
		std::string _body;
		std::string _location;

		ErrorPageGenerator error;
		PathResolver pathResolver;
		FileSystemHandler fileSystemHandler;
		MimeTypeResolver mimeTypeResolver;

		int getStatusCode();
		void setStatusCode(int statusCode);
		int determineStatusCode(const std::string& request, const ServerConfig& config);

		std::string returnGenericErrorResponse(int statusCode, const Request& request, const ServerConfig& config);
		std::string returnRedirectErrorResponse(int statusCode, const Request& request, const ServerConfig& config, const LocationConfig* matchedLocation);

		std::string getStatusLine();
		std::string getContentType();
		size_t getContentLength();
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

		bool isMethodAllowed(const std::string &method, const LocationConfig *loc, const ServerConfig &cfg);
		std::string buildAutoIndexBody(const std::string &uriPath, const std::string &dirFsPath, FileSystemHandler &fs);
		std::string joinPathSimple(const std::string &a, const std::string &b);
		std::string ensureTrailingSlash(const std::string &p);
		std::string ensureLeadingSlash(const std::string &p);
		bool startsWithLocationBoundary(const std::string &uriPath, const std::string &locPath);
		std::string formatHttpDate(std::time_t t);

		static std::string buildFileResponse(const std::string& filePath, const ServerConfig& config);
		static std::string buildErrorResponse(int statusCode, const ServerConfig& config);
		static std::string buildDirectoryListingResponse(const std::string& dirPath, const ServerConfig& config);
		static std::string buildCGIResponse(const std::string& scriptPath, const ServerConfig& config);
		void setStandardHeaders(std::string& response, const std::string& contentType, size_t contentLength);		
};