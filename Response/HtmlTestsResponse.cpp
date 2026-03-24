#include "ResponseBuilder.hpp"

/**
 * 
 * @brief Replaces all occurrences of a placeholder tag with a specific value.
 * @param content The string (HTML body) to modify.
 * @param tag The placeholder to look for (e.g., "{{LISTEN_PORT}}").
 * @param value The real data to insert.
 */
void ResponseBuilder::replaceTag(std::string &content, const std::string &tag, const std::string &value)
{
    if (tag.empty())
        return;

    size_t pos = 0;
    // Find the first occurrence of the tag
    while ((pos = content.find(tag, pos)) != std::string::npos)
    {
        // Replace the tag with the new value
        content.replace(pos, tag.length(), value);
        
        // Advance 'pos' by the length of the new value to avoid infinite loops
        // (in case the value contains the tag itself)
        pos += value.length();
    }
}

/**
 * @brief Builds the html block {{REQUEST_DETAILS}}
 * 
 * @param request 
 */
void ResponseBuilder::insertRequestInfo(const Request& request, const std::string& userInput, const std::string& locPath, const std::string& block)
{
	std::string requestBlock;

	Method method = request.getMethod();
	switch (method)
	{
	case 0:
		requestBlock += "GET";
		break;
	case 1:
		requestBlock += "POST";
		break;
	case 2:
		requestBlock += "DELETE";
		break;
	default:
		requestBlock += "NONE";
		break;
	}
	requestBlock += " ";
	
	requestBlock += request.getPath();
	requestBlock += " ";
	requestBlock += request.getVersion();
	requestBlock += ";\n";

	requestBlock += "Host: ";
	requestBlock += request.getHost();
	requestBlock += ";\n";

	if (!userInput.empty())
	{
		std::string inputCorrected;
		size_t initialPos = 0;
		size_t pos;
		//just a loop to replace all the "%2F" in the userInput for a'/'
		while (true)
		{
			pos = userInput.find("%2F", initialPos);
			if (pos != std::string::npos)
			{
				inputCorrected += userInput.substr(initialPos, pos - initialPos);
				inputCorrected += '/';
				initialPos = pos + 3;
			}
			else
				break;
		}
		inputCorrected += userInput.substr(initialPos);

		requestBlock += "Searched Path: ";
		requestBlock += inputCorrected;
		requestBlock += ";\n";
	}

	if (!locPath.empty())
	{
		requestBlock += "Matched Location: ";
		requestBlock += locPath;
		requestBlock += ";\n";
	}

	requestBlock += "Result: ";
	requestBlock += itostr(getStatusCode());
	requestBlock += " ";
	requestBlock += error.getReasonPhrase(getStatusCode());
	requestBlock += ";\n";

	replaceTag(_body, block, requestBlock);
}

/**
 * @brief Builds the response accordingly to /search request (from index.html)
 * 
 * @param request 
 * @param resolvedConfig 
 * @return void
 */
void ResponseBuilder::insertLocationInfo(const Request& request, const ConfigResolved& resolvedConfig)
{
	if (_contentType != "text/html")
        return;

	std::map<std::string, std::string> queryParams = request.getQueryParams();
	//searches for the userInput in the query path (the name of the input in the html)
	std::string userInput = request.getSpecificQuery("path");
	if (userInput.empty())
	{
		replaceTag(_body, "{{GET_REQUEST_DETAILS}}", "Waiting for a request...");
		replaceTag(_body, "{{LOCATION_DETAILS}}", "Insert a location's path to search its info.");
		return;
	}

	// insert a '/' if the userInput doesn't have one
	if (userInput[0] != '/')
	{
		if (userInput.find("%2F") == 0)
			userInput.erase(0, 3);
		userInput = "/" + userInput;
	}

	//selecting the location block using the userInput to find that match
	const ServerConfig& serverBlock = resolvedConfig.getServerBlock();
	const std::vector<LocationConfig>& locations = serverBlock.getLocations();
	const LocationConfig* selectedLocation = NULL;
	size_t longestMatch = 0;

	for (std::vector<LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		const std::string& locPath = it->getPath();
		if (startsWithLocationBoundary(userInput, locPath) && locPath.size() > longestMatch)
		{
			selectedLocation = &(*it);
			longestMatch = locPath.size();
		}
	}

	if (!selectedLocation)
	{
		_statusCode = 404;
		insertRequestInfo(request, userInput, "", "{{GET_REQUEST_DETAILS}}");
		replaceTag(_body, "{{LOCATION_DETAILS}}", "That location block doesn't exist in the config file.");
		return;
	}

	insertRequestInfo(request, userInput, selectedLocation->getPath(), "{{GET_REQUEST_DETAILS}}");
	std::string locationBlock;
	//add path
	locationBlock += "Path: ";
	locationBlock += selectedLocation->getPath();
	locationBlock += ";\n";

	//add root
	std::string root = selectedLocation->getRoot();
	if (!root.empty())
	{
		locationBlock += "Root: ";
		locationBlock += root;
		locationBlock += ";\n";
	}

	//add alias
	std::string alias = selectedLocation->getAlias();
	if (!alias.empty())
	{
		locationBlock += "Alias: ";
		locationBlock += alias;
		locationBlock += ";\n";
	}

	//add indexes
	const std::vector<std::string>& indexes = selectedLocation->getIndexes();
	if (!indexes.empty())
	{
		locationBlock += "Indexes: ";
		for (size_t i = 0; i < indexes.size(); ++i)
		{
			locationBlock += indexes[i];
			if (i < indexes.size() - 1)
				locationBlock += ", ";
		}
		locationBlock += ";\n";
	}

	//add allowed_methods
	const std::vector<std::string>& methods = selectedLocation->getAllowedMethods();
	locationBlock += "Allowed Methods: ";
	if (!methods.empty())
	{
		for (size_t i = 0; i < methods.size(); ++i)
		{
			locationBlock += methods[i];
			if (i < methods.size() - 1)
				locationBlock += ", ";
		}
		locationBlock += ";\n"; 
	}

	//add autoindex
	if (selectedLocation->getAutoIndexFlag())
	{
		std::string autoindex = selectedLocation->getAutoIndex() ? "On" : "Off";
		locationBlock += "Autoindex: ";
		locationBlock += autoindex;
		locationBlock += ";\n";
	}

	//add returnStatusCode
	int returnCode = selectedLocation->getReturnStatusCode();
	if (returnCode != 0)
	{
		locationBlock += "Return Status Code: ";
		locationBlock += itostr(returnCode);
		locationBlock += ";\n";
	}

	//add returnURL
	std::string returnURL = selectedLocation->getReturnURL();
	if (!returnURL.empty())
	{
		locationBlock += "Return URL: ";
		locationBlock += returnURL;
		locationBlock += ";\n";
	}

	//add returnMessage
	std::string returnMessage = selectedLocation->getReturnMessage();
	if (!returnMessage.empty())
	{
		locationBlock += "Return Message: ";
		locationBlock += returnMessage;
		locationBlock += ";\n";
	}

	//add client_max_body_size
	if (selectedLocation->getMaxBodySizeFlag())
	{
		std::string size = itostr(static_cast<int>(selectedLocation->getMaxBodySize()));
		locationBlock += "Max Body Size: ";
		locationBlock += size;
		locationBlock += ";\n";
	}

	//add cgi
	const std::map<std::string, std::string>& cgi = selectedLocation->getCGI();
	if (!cgi.empty())
	{

		for (std::map<std::string, std::string>::const_iterator it = cgi.begin(); it != cgi.end(); ++it)
		{
			locationBlock += "CGI:";
			locationBlock += " ";
			locationBlock += it->first;
			locationBlock += " ";
			locationBlock += it->second;
			locationBlock += ";";
			locationBlock += "\n";
		}
	}

	//add error_pages
	const std::map<int, std::string>& errorPages = selectedLocation->getErrorPages();
	if (!errorPages.empty())
	{
		locationBlock += "Error Pages:";
		for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
		{
			locationBlock += " ";
			locationBlock += itostr(it->first);
			locationBlock += " ";
			locationBlock += it->second;
			locationBlock += ";";
		}
		locationBlock += "\n";
	}

	//add upload_store
	std::string upload = resolvedConfig.getUploadStore();
	if (!upload.empty())
	{
		locationBlock += "Upload Store: ";
		locationBlock += upload;
		locationBlock += ";\n";
	}
	replaceTag(_body, "{{LOCATION_DETAILS}}", locationBlock);
}

/**
 * @brief Checks server info and builds the block of info for the HTML
 * 
 * @param config The resolved configuration for the current location.
 */
void ResponseBuilder::insertServerInfo(const ConfigResolved& config)
{
    // If it's the index or info page, swap the server block tag
    if (_contentType == "text/html")
    {
		//add ports info
		const std::vector<int>& ports = config.getPorts();
		std::string serverBlock;
		serverBlock += "Ports: ";
		for (size_t i = 0; i < ports.size(); i++)
		{
			serverBlock += itostr(ports[i]);
			if (i < ports.size() - 1)
				serverBlock += ", ";
		}
		serverBlock += ";\n";

		//add root info
		std::string root = config.getRoot();
		serverBlock += "Root: ";
		serverBlock += root;
		serverBlock += ";\n";
		
		//add host info
		std::string host = config.getHost();
		serverBlock += "Host: ";
		serverBlock += host;
		serverBlock += ";\n";

    	//add upload_store
		std::string upload = config.getUploadStore();
		if (upload != "")
		{
			serverBlock += "Upload Store: ";
			serverBlock += upload;
			serverBlock += ";\n";
		}
    	
		//add client_max_body_size
		std::string size = itostr(static_cast<int>(config.getMaxBodySize()));
		serverBlock += "Max Body Size: ";
		serverBlock += size;
		serverBlock += ";\n";

		//add return code
		std::string returnCode = itostr(config.getReturnStatusCode());
    	if (returnCode != "0")
		{
			serverBlock += "Return code: ";
			serverBlock += returnCode;
			serverBlock += ";\n";
		}

		//add autoindex
		std::string autoindex = config.getAutoIndex() ? "On" : "Off";
		serverBlock += "Autoindex: ";
		serverBlock += autoindex;
		serverBlock += ";\n";

		//add allowed_methods
		const std::vector<std::string>& methods = config.getAllowedMethods();
		serverBlock += "Allowed Methods: ";
    	for (size_t i = 0; i < methods.size(); ++i)
		{
        	serverBlock += methods[i];
			if (i < methods.size() - 1)
				serverBlock += ", ";
   		}
    	replaceTag(_body, "{{SERVER_DETAILS}}", serverBlock);
    }
}
