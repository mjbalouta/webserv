#include "LocationConfig.hpp"

LocationConfig::LocationConfig(const std::string& path)
: _path(path), _root(""), _rootDefined(false), _alias(""), _aliasSet(false), _autoIndex(false),
_returnStatusCode(0), _returnURL(""), _returnMessage(""),
_maxBodySize(0), _uploadStore("")
{}

void LocationConfig::setRoot(const std::string& root)
{
	_root = root;
}

void LocationConfig::addIndex(const std::string& index)
{
	_indexes.push_back(index);
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

void LocationConfig::setReturnStatusCode(int code)
{
	_returnStatusCode = code;
}
void LocationConfig::setReturnURL(const std::string& url)
{
	_returnURL = url;
}
int LocationConfig::getReturnStatusCode() const
{
	return _returnStatusCode;
}

const std::string& LocationConfig::getReturnURL() const
{
	return _returnURL;
}

void LocationConfig::setReturnMessage(const std::string& message)
{
	_returnMessage = message;
}
const std::string& LocationConfig::getReturnMessage() const
{
	return _returnMessage;
}

void LocationConfig::addErrorPages(int code, const std::string& file)
{
	_errorPages[code] = file;
}

const std::map<int, std::string>& LocationConfig::getErrorPages() const
{
	return _errorPages;
}

void LocationConfig::addCGI(const std::string& extension, const std::string& extensionPath)
{
	_cgi[extension] = extensionPath;
}

const std::map<std::string, std::string>& LocationConfig::getCGI() const
{
	return _cgi;
}

void LocationConfig::setUploadStore(const std::string& storePath)
{
	_uploadStore = storePath;
}
const std::string& LocationConfig::getUploadStore() const
{
	return _uploadStore;
}

void LocationConfig::clearIndexes()
{
	_indexes.clear();
}

void LocationConfig::setMaxBodySize(unsigned long size)
{
	_maxBodySize = size;
}

unsigned long LocationConfig::getMaxBodySize() const
{
	return _maxBodySize;
}

void LocationConfig::setRootFlag(bool status)
{
	_rootDefined = status;
}

bool LocationConfig::getRootFlag() const
{
	return _rootDefined;
}

