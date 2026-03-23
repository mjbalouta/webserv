#include "ConfigResolved.hpp"

ConfigResolved::ConfigResolved(const Request& request, const ServerConfig& server)
{
	_server = &server;
	_location = findLocationBlock(server, request);
}

/**
 * @brief after finding the server block, we must look for the most accurate match for the request path
 * in the location block's path
 * 
 * @param server 
 * @param request 
 * @return const LocationConfig* 
 */
const LocationConfig* ConfigResolved::findLocationBlock(const ServerConfig& server, const Request& request)
{
	std::string path = request.getPath();

	const std::vector<LocationConfig>& locationBlocks = server.getLocations();
	const LocationConfig* bestMatch = NULL;
	size_t longestMatchSize = 0;

	for (std::vector<LocationConfig>::const_iterator locationIt = locationBlocks.begin(); locationIt != locationBlocks.end(); ++locationIt)
	{
		std::string locationPath = locationIt->getPath();
		//finding the longest match (if the path is /images/more/more, we have to search for the best possible
		// match - it can be just /images, but if there is a path /images/more, we have to select this last one)
		if (path.find(locationPath) == 0)
		{
			//it needs to be a full match (can't match /images2 with /images) so we need to check these:
			bool isFullMatch = (locationPath == "/" || path.size() == locationPath.size()
								|| locationPath[locationPath.size() - 1] == '/'
								|| path[locationPath.size()] == '/');
			if (isFullMatch && locationPath.size() > longestMatchSize)
			{
				bestMatch = &(*locationIt);
				longestMatchSize = locationPath.size();
			}
		}
	}
	return bestMatch;
}

/**
 * @brief Checks if a location block exists and returns its path
 * 
 * @return const std::string 
 */
std::string ConfigResolved::getLocationPath() const
{
	if (_location && !_location->getPath().empty())
		return _location->getPath();
	return "";
}

/**
 * @brief Checks if location exists and if it has a root: if it has, returns it, if it does not,
 * it returns the server's root instead
 * 
 * @return const std::string 
 */
const std::string& ConfigResolved::getRoot() const
{
	if (_location && !_location->getRoot().empty())
		return _location->getRoot();
	return _server->getRoot();
}

/**
 * @brief Checks if alias exists, if it exists returns it, if not, returns empty
 * 
 * @return const std::string 
 */
std::string ConfigResolved::getAlias() const
{
	if (_location && !_location->getAlias().empty())
		return _location->getAlias();
	return ""; //in c++ we can't return NULL, it has to return an empty string
}

/**
 * @brief Joins the serverPath with the requestPath in order to obtain the resolvedPath
 * (the actual location on the computer's hard drive where a file is stored).
 * Handles the difference between alias and root: when alias exists, the resolvedPath is going to be
 * the serverPath + requestPath without location's path prefix.
 * 
 * @param request 
 * @return const std::string 
 */
std::string ConfigResolved::getResolvedPath(const Request& request) const
{
	std::string absolutePath = getAbsolutePath();
	std::string alias = this->getAlias();
	std::string root = this->getRoot();
	std::string serverPath;
	std::string requestPath = request.getPath(); //URI

	if (!alias.empty())
	{
		if (requestPath.find(this->getLocationPath()) == 0)
	 		requestPath.erase(0, this->getLocationPath().size());
		if (alias[0] == '/')
			serverPath = alias;
		else
			serverPath = absolutePath + alias;
	}
	else if (!root.empty())
	{
		if (root[0] == '/')
			serverPath = root;
		else
			serverPath = absolutePath + root;
	}
	else
		serverPath = absolutePath;

	if (serverPath.find_last_of('/') != serverPath.size() - 1)
		serverPath += '/';
	if (requestPath.find_first_of('/') == 0)
		requestPath.erase(0, 1);

	return serverPath + requestPath;
}

/**
 * @brief If indexes exist in location, returns those, if not, returns the server ones
 * 
 * @return const std::vector<std::string>& 
 */
const std::vector<std::string>& ConfigResolved::getIndexes() const
{
	if (_location && !_location->getIndexes().empty())
		return _location->getIndexes();
	return _server->getIndexes();
}

/**
 * @brief Decides which allowed methods to return between location and server
 * 
 * @return const std::vector<std::string>& 
 */
const std::vector<std::string>& ConfigResolved::getAllowedMethods() const
{
	if (_location && !_location->getAllowedMethods().empty())
		return _location->getAllowedMethods();
	return _server->getAllowedMethods();
}

/**
 * @brief Decides which map of error pages to return
 * 
 * @return const std::map<int, std::string>& 
 */
const std::map<int, std::string>& ConfigResolved::getErrorPages() const
{
	if (_location && !_location->getErrorPages().empty())
		return _location->getErrorPages();
	return _server->getErrorPages();
}

/**
 * @brief Returns the server host
 * 
 * @return const std::string 
 */
const std::string& ConfigResolved::getHost() const
{
	return _server->getHost();
}

/**
 * @brief Returns server's server_names
 * 
 * @return const std::vector<std::string>& 
 */
const std::vector<std::string>& ConfigResolved::getServerNames() const
{
	return _server->getServerNames();
}

/**
 * @brief Decides which client_max_body_size to return
 * 
 * @return unsigned long 
 */
unsigned long ConfigResolved::getMaxBodySize() const
{
	if (_location && _location->getMaxBodySizeFlag())
		return _location->getMaxBodySize();
	return _server->getMaxBodySize();
}

/**
 * @brief Decides wether to return location's or server's autoindex.
 * 
 * @return true 
 * @return false 
 */
bool ConfigResolved::getAutoIndex() const
{
	if (_location && _location->getAutoIndexFlag())
		return _location->getAutoIndex();
	return _server->getAutoIndex();
}

/**
 * @brief Returns the location's status code or 0 if location doesn't exist
 * 
 * @return int 
 */
int ConfigResolved::getReturnStatusCode() const
{
	if (_location)
		return _location->getReturnStatusCode();
	return 0;
}

/**
 * @brief Returns the URL from location or empty string if it doesn't exist
 * 
 * @return const std::string 
 */
std::string ConfigResolved::getReturnURL() const
{
	if (_location)
		return _location->getReturnURL();
	return "";
}

/**
 * @brief Returns the return message if it exists, if not returns empty string
 * 
 * @return const std::string 
 */
std::string ConfigResolved::getReturnMessage() const
{
	if (_location)
		return _location->getReturnMessage();
	return "";
}

/**
 * @brief Returns location's CGI
 * 
 * @return const std::map<std::string, std::string>& 
 */
const std::map<std::string, std::string>& ConfigResolved::getCgi() const
{
	if (_location)
		return _location->getCGI();
	
	//return a static empty map to avoid returning a reference to a temporary local variable
	//static allows this map to live outside the scope of this function, which avoids crashing
	static std::map<std::string, std::string> emptyMap;
	return emptyMap;
}

/**
 * @brief Returns upload_store
 * 
 * @return const std::string 
 */
std::string ConfigResolved::getUploadStore() const
{
	if (_location)
		return _location->getUploadStore();
	return "";
}

const std::string& ConfigResolved::getAbsolutePath() const
{
	return _server->getAbsolutePath();
}