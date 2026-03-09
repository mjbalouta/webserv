#include "LocationConfig.hpp"

LocationConfig::LocationConfig(const std::string& path)
: _path(path), _autoIndex(false), _aliasSet(false), _returnCode(0), _maxBodySize(0)
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

void LocationConfig::setAlias(const std::string& alias)
{
	_alias = alias;
}

void LocationConfig::setAliasFlag(bool status)
{
	_aliasSet = status;
}

const std::string& LocationConfig::getAlias() const
{
	return _alias;
}

bool LocationConfig::getAliasFlag() const
{
	return _aliasSet;
}
