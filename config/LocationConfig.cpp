#include "LocationConfig.hpp"

LocationConfig::LocationConfig(const std::string& path)
: _path(path), _autoIndex(false)
{}

void LocationConfig::setRoot(const std::string& root)
{
	_root = root;
}

void LocationConfig::setIndexes(const std::vector<std::string>& indexes)
{
	_indexes = indexes;
}

void LocationConfig::addAllowedMethod(const std::string& allowedMethod)
{
	_allowedMethods.push_back(allowedMethod);
}

void LocationConfig::setAutoIndex(bool autoIndex)
{
	_autoIndex = autoIndex;
}

const std::string& LocationConfig::getPath() const
{
	return _path;
}

const std::string& LocationConfig::getRoot() const
{
	return _root;
}

const std::vector<std::string>& LocationConfig::getIndexes() const
{
	return _indexes;
}

const std::vector<std::string>& LocationConfig::getAllowedMethods() const
{
	return _allowedMethods;
}

bool LocationConfig::getAutoIndex() const
{
	return _autoIndex;
}
