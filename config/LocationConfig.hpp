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
	int _returnCode;
	std::string _returnURL;
	unsigned long _maxBodySize; //if it is set to 0, use server size
	//CGI??
	std::map<std::string, std::string> _cgi;

	public:
	LocationConfig(const std::string& path);

	//path doesn't need a set because it is always fixed for a location
	void setRoot(const std::string& root);
	void setIndexes(const std::vector<std::string>& indexes);
	void addAllowedMethod(const std::string& allowedMethod);
	void setAutoIndex(bool autoIndex);
	void setAlias(const std::string& alias);
	void setAliasFlag(bool status);

	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getAllowedMethods() const;
	bool getAutoIndex() const;
	const std::string& getAlias() const;
	bool getAliasFlag() const;

};