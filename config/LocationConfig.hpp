#pragma once

#include "Includes.hpp"

class LocationConfig
{
	private:
	std::string _path; //location /images -> the path here would be /images
	std::string _root; //location root overrides server root
	std::string _alias; //used to "overwrite" the path when looking for comething
	bool _aliasSet; //used to identify if alias is set: if it is set, root is ignored
	std::vector<std::string> _indexes; //the default file to serve when a directory is requested
	std::vector<std::string> _allowedMethods;
	bool _autoIndex; //represents wether the server should generate a directory listing
	int _returnStatusCode; //for return attribute
	std::string _returnURL; //for return attribute
	std::string _returnMessage; //for return attribute
	unsigned long _maxBodySize; //if it is set to 0, use server size
	std::map<std::string, std::string> _cgi; //CGI??
	std::map<int, std::string> _errorPages; //custom HTML pages for HTTP errors
	std::string _uploadStore; //path to store the upload files

	public:
	LocationConfig(const std::string& path);

	//path doesn't need a set because it is always fixed for a location
	void setRoot(const std::string& root);
	void setIndexes(const std::vector<std::string>& indexes);
	void addAllowedMethod(const std::string& allowedMethod);
	void setAutoIndex(bool autoIndex);
	void setAlias(const std::string& alias);
	void setAliasFlag(bool status);
	void setReturnStatusCode(int code);
	void setReturnURL(const std::string& url);
	void setReturnMessage(const std::string& message);
	void addErrorPages(int code, const std::string& file);
	void addCGI(const std::string& extension, const std::string& extensionPath);
	void setUploadStore(const std::string& storePath);

	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const; // Retorna o vetor para quando o pedido for um diretório
	const std::vector<std::string>& getAllowedMethods() const;
	bool getAutoIndex() const;
	const std::string& getAlias() const;
	bool getAliasFlag() const;
	int getReturnStatusCode() const;
	const std::string& getReturnURL() const;
	const std::string& getReturnMessage() const;
	const std::map<int, std::string>& getErrorPages() const; // Retorna o map para poderes verificar se o erro existe
	const std::map<std::string, std::string>& getCGI() const;
	const std::string& getUploadStore() const; 
};