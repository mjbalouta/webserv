#include "ConfigResolved.hpp"

ConfigResolved::ConfigResolved(const ConfigParser& config, const Request& request, const ServerConfig& server)
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
			if (locationPath.size() > longestMatchSize)
			{
				bestMatch = &(*locationIt);
				longestMatchSize = locationPath.size();
			}
		}
	}
	return bestMatch;
}

/**
 * @brief Checks if location exists and if it has a root: if it has, returns it, if it does not,
 * it returns the server's root instead
 * 
 * @return const std::string 
 */
const std::string ConfigResolved::getRoot() const
{
	if (_location && !_location->getRoot().empty())
		return _location->getRoot();
	return _server->getRoot();
}

/**
 * @brief Joins the serverPath with the requestPath in order to obtain the resolvedPath
 * (the actual location on the computer's hard drive where a file is stored.)
 * 
 * @param request 
 * @return const std::string 
 */
const std::string ConfigResolved::getResolvedPath(const Request& request) const
{
	std::string serverPath = this->getRoot();
	std::string requestPath = request.getPath(); //same as URI

	if (serverPath.empty())
		return requestPath;

	if (serverPath.find_last_of('/') != serverPath.size() - 1)
		serverPath += '/';
	if (requestPath.find_first_of('/') == 0)
		requestPath.erase(0, 1);
	std::string resolvedPath = serverPath + requestPath;
	return resolvedPath;
}