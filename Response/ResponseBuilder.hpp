#pragma once

#include "../Includes.hpp"
#include "../Utils.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../ServerManager/Request.hpp"
#include "../fileResourceManagement/PathResolver.hpp"
#include "../fileResourceManagement/FileSystemHandler.hpp"
#include "../fileResourceManagement/MimeTypeResolver.hpp"
#include "../fileResourceManagement/ErrorPageGenerator.hpp"
#include "../routing/ConfigResolved.hpp"
#include <algorithm>

typedef struct MultipartData {
	std::string boundary;
	std::map<std::string, std::string> parts; // pair<headers, body>
} MultipartData;

class ResponseBuilder{
	public:
		std::string returnResponse(const Request& request, const ConfigResolved& resolvedConfig, bool keepAlive);
		std::string returnGenericErrorResponse(int statusCode, const Request& request, const ConfigResolved& resolvedConfig);
		std::string returnRedirectErrorResponse(int statusCode, const Request& request, const ConfigResolved& resolvedConfig);
	
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

//		status code determination
		int determineStatusCode(const std::string& request, const ConfigResolved& resolvedConfig);

// 		Helper functions for response building
		std::string getStatusCodeString();
		std::string getStatusLine();
		std::string getContentType();
		size_t getContentLength();
		std::string getContentLengthString();
		std::time_t getLastModified();
		size_t getDate();
		std::string getDateString();
		std::string getLastModifiedString();
		std::string getVersionString(const Request& request) const;

//		Setters for building response
/* 		void setStatusLine(int statusCode);
		void setContentType(const std::string& filePath);
		void setContentLength(size_t contentLength);
		void setDate(std::time_t date);
		void setLastModified(std::time_t lastModified);
		void setBody(const std::string& body); */

// 		Helper functions for response building
		bool isMethodAllowed(const std::string &method, const ConfigResolved &resolvedConfig);
		std::string buildAutoIndexBody(const std::string &uriPath, const std::string &dirFsPath, FileSystemHandler &fs);
		std::string joinPathSimple(const std::string &a, const std::string &b);
		std::string ensureTrailingSlash(const std::string &p);
		std::string ensureLeadingSlash(const std::string &p);
		bool startsWithLocationBoundary(const std::string &uriPath, const std::string &locPath);
		std::string formatHttpDate(std::time_t t);

//		Multipart form data parsing
		MultipartData parseMultipartFormData(const std::string& body, const std::string& boundary);
		std::string extractFilenameFromPartHeaders(const std::string& headers);
		std::string sanitizeFilename(const std::string& filename);
		std::string findFilenameFromHeaders(const std::map<std::string, std::string>& headers);
		std::string findFilenameContent(const std::map<std::string, std::string>& headers);


// 		Response builders for different scenarios
		std::string buildDeleteResponse(const Request& request, const std::string& fileSystemPath, const ConfigResolved& resolvedConfig);
		std::string buildPostResponse(const Request& request, const ConfigResolved& resolvedConfig);
		std::string buildRedirectResponse(const Request &request, const ConfigResolved &matched);
		std::string buildFileResponse(const Request& request, const std::string& filePath, const ConfigResolved& resolvedConfig);
		std::string buildDirectoryListingResponse(const Request& request, const std::string& uriPath, const std::string& dirFsPath, const ConfigResolved& resolvedConfig);
		//std::string buildCGIResponse(const std::string& scriptPath, const ConfigResolved& resolvedConfig);
		void setStandardHeaders(std::string& response, const std::string& contentType);
		void replaceTag(std::string &content, const std::string &tag, const std::string &value);
		void insertServerInfo(const ConfigResolved& config);
		void insertLocationInfo(const Request& request, const ConfigResolved& resolvedConfig);
		void insertRequestInfo(const Request& request, const std::string& userInput, const std::string& locPath, const std::string& block);
		void listGalleryFiles(const ConfigResolved& config);
	//	std::string buildCGIResponse(const std::string& scriptPath, const std::string& executor, const Request& request, const ConfigResolved& resolvedConfig);
};