#include "CGIHandler.hpp"
/* 
bool CGIHandler::isCgiRequest(const Request& request, const ConfigResolved& resolvedConfig, std::string& outScriptPath, std::string& outExecutor)
{
	if (resolvedConfig.getCgi().empty())
		return false;
	size_t pos = outScriptPath.find_last_of('.');
	if (pos == std::string::npos)
		return false;
	std::string cgiExtension = outScriptPath.substr(pos);
	if (cgiExtension.empty())
		return false;
	std::map<std::string, std::string> cgiMap = resolvedConfig.getCgi();
	if (cgiMap.empty())
		return false;
	std::map<std::string, std::string>::const_iterator it = cgiMap.find(cgiExtension);
	if (it == cgiMap.end())
		return false;
	outExecutor = it->second;
	return true;
}


std::map<std::string, std::string> CGIHandler::buildCGIEnv(const Request& request, const ConfigResolved& resolvedConfig, const std::string& scriptPath)
{
	std::map<std::string, std::string> env;
	// Initialize CGI environment variables
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	env["SERVER_PORT"] = resolvedConfig.getPort();
	env["SCRIPT_NAME"] = request.getPath();
	env["SCRIPT_FILENAME"] = scriptPath;
	env["SERVER_PROTOCOL"] = request.getVersion();
	env["REQUEST_METHOD"] = request.getMethodStr();
	if (request.getMethodStr() == "POST" && !request.getBody().empty())
	{
		env["CONTENT_LENGTH"] = request.getHeader("content-length");
		env["CONTENT_TYPE"] = request.getHeader("content-type");
	}
	
	for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it)
	{
		std::string envName = requestHeadtoCGIEnv(it->first);
		env[envName] = it->second;
	}
	
	std::string query_string;
	for (std::map<std::string, std::string>::const_iterator it = request.getQueryParams().begin(); it != request.getQueryParams().end(); ++it)
		query_string += it->first + "=" + it->second + "&";
	if (!query_string.empty())
		query_string.erase(query_string.size() - 1);
	env["QUERY_STRING"] = query_string;

	return env;
}

std::string CGIHandler::requestHeadtoCGIEnv(const std::string& header){
	std::string result = "HTTP_" + header;
	std::replace(result.begin(), result.end(), '-', '_');
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}


std::string CGIHandler::buildCGIResponse(const std::string& scriptPath, const std::string& executor, const Request& request, const ConfigResolved& resolvedConfig)
{
	std::string cgiExtension = scriptPath.substr(scriptPath.find_last_of('.'));
	std::map<std::string, std::string> cgiMap = resolvedConfig.getCgi();
/* 	if (cgiMap.end() == cgiMap.find(cgiExtension))
		return returnGenericErrorResponse(500, request, resolvedConfig); */
	CGIEnv = buildCGIEnv(request, resolvedConfig, scriptPath);
	return "";
} */