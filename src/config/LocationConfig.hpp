#pragma once

#include "Includes.hpp"

class LocationConfig
{
	private:
	std::string _path; //location /images -> the path here would be /images
	std::string _root; //location root overrides server root
	std::string _index; //the default file to serve when a directory is requested
	std::vector<std::string> _allowedMethods;
	bool _autoIndex; //represents wether the server should generate a directory listing
};