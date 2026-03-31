#include "CGIHandler.hpp"
#include "../fileResourceManagement/ErrorPageGenerator.hpp"

/**
 * @brief Builds the envp array passed to execve().
 *
 * envStrings owns the heap storage for each "KEY=VALUE" string.
 * envp holds raw char* pointers into those strings plus a NULL terminator.
 * Both vectors must stay alive until after execve() is called in the child.
 */
void CgiHandler::buildEnv(const Request &request,
						  const std::string &scriptPath,
						  const ServerConfig &server,
						  std::vector<std::string> &envStrings,
						  std::vector<char *> &envp)
{
	envStrings.clear();
	envp.clear();

	envStrings.push_back("REQUEST_METHOD=" + request.getMethodStr());
	std::string queryString;
	const std::map<std::string, std::string> &params = request.getQueryParams();
	for (std::map<std::string, std::string>::const_iterator it = params.begin(); it != params.end(); ++it) {
		if (!queryString.empty())
			queryString += "&";
		queryString += it->first + "=" + it->second;
	}
	envStrings.push_back("QUERY_STRING=" + queryString);	envStrings.push_back("SCRIPT_FILENAME=" + scriptPath);
	envStrings.push_back("PATH_INFO=" + request.getPath());
	envStrings.push_back("PATH_TRANSLATED=" + scriptPath);
	envStrings.push_back("SERVER_PROTOCOL=" + request.getVersion());
	envStrings.push_back("SERVER_NAME=" + server.getHost());
	envStrings.push_back("SERVER_PORT=" + itostr(server.getPorts()[0]));
	envStrings.push_back("HTTP_HOST=" + request.getHost());

	// REDIRECT_STATUS is required by PHP-CGI; harmless for Python/shell
	envStrings.push_back("REDIRECT_STATUS=200");

	// POST-specific variables — safe to add for all methods; empty string is fine
	envStrings.push_back("CONTENT_TYPE=" + request.getHeader("content-type"));
	envStrings.push_back("CONTENT_LENGTH=" + request.getHeader("content-length"));

	// Build the NULL-terminated char* array for execve()
	for (size_t i = 0; i < envStrings.size(); ++i)
		envp.push_back(const_cast<char *>(envStrings[i].c_str()));
	envp.push_back(NULL);
}

/**
 * @brief Creates pipes, forks, and execs the CGI script.
 *
 * Pipe layout:
 *   inPipe[0]  → child  STDIN   (child reads POST body)
 *   inPipe[1]  → parent writes  (parent feeds POST body)
 *   outPipe[0] → parent reads   (parent captures CGI output)
 *   outPipe[1] → child  STDOUT  (child writes response)
 *
 * All four ends are set non-blocking before fork() so that if the parent
 * inherits any of them after execve failure, they will not block.
 *
 * In the child:
 *   dup2 wires inPipe[0] → STDIN and outPipe[1] → STDOUT,
 *   then ALL original pipe fds are closed before execve.
 *   On execve failure _exit(1) is used — NOT exit() — to avoid
 *   flushing parent stdio buffers which would corrupt state.
 *
 * In the parent:
 *   inPipe[0] and outPipe[1] are closed immediately (child's ends).
 *   The caller receives readFd=outPipe[0] and writeFd=inPipe[1].
 */
CgiProcess CgiHandler::start(const Request &request, const std::string &scriptPath, const std::string &interpreter, const ServerConfig &server)
{
	int inPipe[2];
	int outPipe[2];

	if (pipe(inPipe) < 0)
		throw std::runtime_error("pipe() failed for CGI stdin");
	if (pipe(outPipe) < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		throw std::runtime_error("pipe() failed for CGI stdout");
	}

	// Set all four ends non-blocking before fork so the parent never blocks
	// on pipe I/O in the epoll loop, and the child inherits safe fds too.
	try {
		setNonBlockingFd(inPipe[0]);
		setNonBlockingFd(inPipe[1]);
		setNonBlockingFd(outPipe[0]);
		setNonBlockingFd(outPipe[1]);
	} catch (...) {
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		throw;
	}

	// Build env before fork so memory is ready in the parent's address space.
	// After fork the child gets a copy; execve replaces the process image.
	std::vector<std::string> envStrings;
	std::vector<char *> envp;
	buildEnv(request, scriptPath, server, envStrings, envp);

	// Build args: { interpreter, scriptPath, NULL }
	// The interpreter is argv[0]; the script path is argv[1].
	char *args[3];
	args[0] = const_cast<char *>(interpreter.c_str());
	args[1] = const_cast<char *>(scriptPath.c_str());
	args[2] = NULL;

	pid_t pid = fork();
	if (pid < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		throw std::runtime_error("fork() failed for CGI");
	}

	// - Child process
	if (pid == 0)
	{
		// Wire inPipe read-end to STDIN so the script reads POST body from it
		if (dup2(inPipe[0], STDIN_FILENO) < 0)
			_exit(1); //USE KILL INSTEAD (exit is forbidden)
		// Wire outPipe write-end to STDOUT so print()/echo go into our pipe
		if (dup2(outPipe[1], STDOUT_FILENO) < 0)
			_exit(1); //USE KILL INSTEAD (exit is forbidden)

		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		execve(interpreter.c_str(), args, envp.data());
		_exit(1); //USE KILL INSTEAD (exit is forbidden)
	}

	// - Parent process 
	close(inPipe[0]);   // child reads from this
	close(outPipe[1]);  // child writes to this

	CgiProcess cgi;
	cgi.pid = pid;
	cgi.writeFd = inPipe[1];   // parent write here
	cgi.readFd = outPipe[0];  // parent read output here
	cgi.startTime = time(NULL);

	return cgi;
}

std::string CgiHandler::buildResponse(const std::string &rawOutput, const std::string &httpVersion, bool keepAlive)
{
	if (rawOutput.empty())
	{
		ErrorPageGenerator error;
		std::string statusLine = httpVersion + " 500 " + error.getReasonPhrase(500) + "\r\n";
		std::string contentType = "text/html";
		std::string body = error.generateErrorPage(500, "Internal Server Error");
		std::stringstream ss;
		ss << body.size();
		std::string response = statusLine;
		response += "Content-Type: " + contentType + "\r\n";
		response += "Content-Length: " + ss.str() + "\r\n";
		response += std::string("Connection: ") + (keepAlive ? "keep-alive" : "close") + "\r\n";
		response += "\r\n";
		response += body;
		return response;
	}
	if (rawOutput.find("HTTP/") == 0)
	{
		std::string response = rawOutput;
		std::string sep = "\r\n\r\n";
		size_t pos = response.find(sep);
		if (pos == std::string::npos)
		{
			sep = "\n\n";
			pos = response.find(sep);
		}
		if (pos != std::string::npos)
		{
			std::string headers = response.substr(0, pos);
			std::string body = response.substr(pos + sep.size());

			if (toLower(headers).find("connection:") == std::string::npos)
			{
				headers += "\r\nConnection: " + std::string(keepAlive ? "keep-alive" : "close");
				response = headers + "\r\n\r\n" + body;
			}
		}
		return response;
	}

	std::string normalized = rawOutput;
	size_t p = 0;
	while ((p = normalized.find("\r\n", p)) != std::string::npos)
		normalized.replace(p, 2, "\n");
	for (size_t i = 0; i < normalized.size(); ++i)
	{
		if (normalized[i] == '\r')
			normalized[i] = '\n';
	}

	const std::string blankLine = "\n\n";
	size_t sepPos = normalized.find(blankLine);
	if (sepPos == std::string::npos)
	{
		std::string contentType = "text/plain";
		return httpVersion + " 200 OK\r\nContent-Type: " + contentType
			+ "\r\nContent-Length: " + itostr(normalized.size())
			+ "\r\nConnection: " + std::string(keepAlive ? "keep-alive" : "close")
			+ "\r\n\r\n" + normalized;
	}
	std::vector<std::pair<std::string, std::string> > headers;
	{
		std::string headersPart = normalized.substr(0, sepPos);
		std::string bodyPart = normalized.substr(sepPos + blankLine.size());

			std::istringstream headerStream(headersPart);
			std::string line;
			while (std::getline(headerStream, line, '\n'))
			{
				size_t colonPos = line.find(":");
				if (colonPos != std::string::npos)
				{
					std::string key = line.substr(0, colonPos);
					std::string value = line.substr(colonPos + 1);
					trimSpaces(key);
					trimSpaces(value);
					headers.push_back(std::make_pair(key, value));
				}
			}
			if (headers.empty())
				headers.push_back(std::make_pair("Content-Type", "text/plain"));
			std::string headerResponse;
			bool hasStatusLine = false;
			for (size_t i = 0; i < headers.size(); ++i)
			{
				std::string keyLower = toLower(headers[i].first);
				if (keyLower == "status")
				{
					headerResponse += httpVersion + " " + headers[i].second + "\r\n";
					hasStatusLine = true;
					continue;
				}
				if (keyLower == "content-length")
				{
					std::stringstream ss;
					ss << bodyPart.size();
					headers[i].second = ss.str();
				}
				headerResponse += headers[i].first + ": " + headers[i].second + "\r\n";
			}
			if (toLower(headerResponse).find("content-length:") == std::string::npos)
			{
				std::stringstream ss;
				ss << bodyPart.size();
				headerResponse += "Content-Length: " + ss.str() + "\r\n";
			}
			if (toLower(headerResponse).find("connection:") == std::string::npos)
				headerResponse += "Connection: " + std::string(keepAlive ? "keep-alive" : "close") + "\r\n";
			std::string response;
			if (!hasStatusLine)
				response = httpVersion + " 200 OK\r\n" + headerResponse + "\r\n" + bodyPart;
			else
				response = headerResponse + "\r\n" + bodyPart;
			return response;
	}
	return httpVersion + " 200 OK\r\nContent-Type: text/plain\r\nConnection: " + std::string(keepAlive ? "keep-alive" : "close") + "\r\n\r\n" + normalized;
}