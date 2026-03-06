#pragma once

#include "Includes.hpp"

class LocationConfig
{
	private:
	std::string _path; //location /images -> the path here would be /images
	std::string _root; //location root overrides server root
	std::vector<std::string> _indexes; //the default file to serve when a directory is requested
	std::vector<std::string> _allowedMethods;
	bool _autoIndex; //represents wether the server should generate a directory listing

	public:
	LocationConfig(const std::string& path);

	//path doesn't need a set because it is always fixed for a location
	void setRoot(const std::string& root);
	void setIndexes(const std::vector<std::string>& indexes);
	void addAllowedMethod(const std::string& allowedMethod);
	void setAutoIndex(bool autoIndex);

	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getAllowedMethods() const;
	bool getAutoIndex() const;

};