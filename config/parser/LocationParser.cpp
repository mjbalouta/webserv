#include "ConfigParser.hpp"

/**
 * @brief Checks information after 'allow_methods' keyword
 * 
 * (Allow_methods directive: lists the HTTP methods allowed for this location (GET, POST, DELETE).)
 * 
 * @param location 
 */
void ConfigParser::parseAllowMethods(LocationConfig& location)
{
	location.clearMethods();

	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definitions after 'allow_methods' keyword.");

	while(_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
	{
		token = _tokens[_currentToken];
		if (token != "GET" && token != "POST" && token != "DELETE")
			throw ConfigException("Error: Invalid method: " + token);
		location.addAllowedMethod(token);
		++_currentToken;
	}

	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'allow_methods' definition.");
}


/**
 * @brief Validates information after 'upload_store' keyword
 * 
 * (Upload_store directive: configures the directory path where uploaded files will be stored.
 * This directive is used during POST requests with multipart/form-data. It defines the destination
 * on the local file system where the server will create and save the uploaded files.)
 * 
 * @param location 
 */
void ConfigParser::parseUploadStore(LocationConfig& location)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definitions after 'upload_store' keyword.");

	ConfigUtils::checkIfDirectory(_tokens[_currentToken]);
	std::string storePath = _tokens[_currentToken];
	if (storePath[0] != '/')
		storePath = _absolutePath + storePath;
	location.setUploadStore(storePath);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'upload_store' definitions.");
}

/**
 * @brief Validates information after 'cgi_pass' keyword
 * 
 * (Cgi_pass directive: maps a file extension to a specific CGI executable path.
 * This directive tells the server which program to use when executing scripts. It enables
 * the processing of dynamic content by passing the request to an external script.)
 * 
 * @param location 
 */
void ConfigParser::parseCGI(LocationConfig& location)
{
	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing definitions after 'cgi_pass' keyword.");

	ConfigUtils::checkExtension(_tokens[_currentToken]);
	std::string extension = _tokens[_currentToken];

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] == ";")
		throw ConfigException("Error: Missing path for the executor: " + _tokens[_currentToken - 1]);

	std::string extensionPath = _tokens[_currentToken];
	if (extensionPath[0] != '/')
		extensionPath = _absolutePath + extensionPath;

	ConfigUtils::validatePath(extensionPath);
	ConfigUtils::CheckInterpreter(extensionPath);

	ConfigUtils::checksIfAlreadyExists(extensionPath, location);
	location.addCGI(extension, extensionPath);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'cgi_pass' definitions.");
}

/**
 * @brief Validates information after 'return' keyword
 * 
 * (Return directive: configures an HTTP redirection for the location.
 * When a return is triggered, the server stops normal processing and sends an HTTP
 * redirect response with the provided status code and the 'Location' header pointing
 * to the new URL.)
 * 
 * @param location 
 */
void ConfigParser::parseReturn(LocationConfig& location)
{
	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definitions after 'return' keyword.");

	if (!allDigits(token))
		throw ConfigException("Error: Missing HTTP status code after 'return' keyword.");
	if (token.size() != 3)
		throw ConfigException("Error: Invalid HTTP status code format: " + token);
	int code = atoi(token.c_str());
	location.setReturnStatusCode(code);
	++_currentToken;
	checkIfTokenExists();
	//codes inside the range 300-399 must have an URL next to it
	if (code >= 300 && code <= 399)
	{
		if (_tokens[_currentToken] == ";")
			throw ConfigException("Error: Missing URL for status code: " + _tokens[_currentToken - 1]);
		ConfigUtils::validateURL(_tokens[_currentToken]);
		location.setReturnURL(_tokens[_currentToken]);
		++_currentToken;
	}
	//codes outside that range can have a message next to it or not
	else if (_tokens[_currentToken] != ";")
	{
		std::string message;
		while (_currentToken < _tokens.size() && _tokens[_currentToken] != ";")
		{
			ConfigUtils::validateMessage(_tokens[_currentToken]);
			if (!message.empty())
				message += " ";
			message += _tokens[_currentToken];
			++_currentToken;
		}
		location.setReturnMessage(message);
	}
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after 'return' definition.");
}

/**
 * @brief Checks information after 'alias' token
 * 
 * (Alias directive: defines a replacement for the specific location's path. Unlike 'root',
 * where the location path is appended to the root string, 'alias' completely replaces the part
 * of the URI that matches the location with the specific file system path.)
 * 
 * @param location 
 */
void ConfigParser::parseAlias(LocationConfig& location)
{
	//check if root is defined (they cannot exist both)
	if (!location.getRoot().empty())
		throw ConfigException("Error: Root and alias cannot coexist in a location block.");
	//check if alias already exists
	if (location.getAliasFlag() == true)
		throw ConfigException("Error: Alias cannot be redefined.");

	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	if (token == ";")
		throw ConfigException("Error: Missing definition after 'alias' keyword.");

	ConfigUtils::validatePath(token);
	if (token[0] != '/')
		token = _absolutePath + token;

	location.setAlias(token);
	location.setAliasFlag(true);

	++_currentToken;
	checkIfTokenExists();
	if (_tokens[_currentToken] != ";")
		throw ConfigException("Error: Expected a ';' after alias definition.");
}

/**
 * @brief Main function to parse location blocks
 * 
 * @param server 
 */
void ConfigParser::parseLocation(ServerConfig& server)
{
	++_currentToken;
	checkIfTokenExists();

	std::string token = _tokens[_currentToken];
	//mandatory format: 'location' + 'path' + {
	if (token == "{")
		throw ConfigException("Error: Expected a path after 'location' keyword.");

	ConfigUtils::validatePath(token);
	LocationConfig location(token);

	++_currentToken;
	checkIfTokenExists();

	if (_tokens[_currentToken] != "{")
		throw ConfigException("Error: Expected '{' token after location's path.");

	int endBracket = 0;
	while (++_currentToken < _tokens.size())
	{
		std::string token = _tokens[_currentToken];
		if (token == "}")
		{
			endBracket = 1;
			break;
		}
		else if (token == "root")
			parseRoot(location);
		else if (token == "alias")
			parseAlias(location);
		else if (token == "allow_methods")
			parseAllowMethods(location);
		else if (token == "client_max_body_size")
			parseMaxBodySize(location);
		else if (token == "index")
			parseIndex(location);
		else if (token == "autoindex")
			parseAutoindex(location);
		else if (token == "return")
			parseReturn(location);
		else if (token == "cgi_pass") //verificar se cgi_pass e suficiente
			parseCGI(location); 
		else if (token == "error_page")
			parseErrorPage(location);
		else if (token == "upload_store") // para o metodo POST
			parseUploadStore(location);	
		else
			throw ConfigException("Error: Unknown keyword " + _tokens[_currentToken]);
	}
	if (!endBracket)
		throw ConfigException("Error: Expected '}' in the end of location block.");
	server.addLocation(location);
}