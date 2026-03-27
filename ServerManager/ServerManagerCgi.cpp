#include "ServerManager.hpp"

/**
 * @brief Starts a CGI child process and registers its pipe fds in epoll.
 *
 * Delegates fork/pipe/execve to CgiHandler::start().
 * Registers:
 *   - cgi.readFd  in epoll with EPOLLIN  (always — we always want output)
 *   - cgi.writeFd in epoll with EPOLLOUT (POST only — there is a body to feed)
 * For GET requests cgi.writeFd is closed immediately so the child gets EOF
 * on its STDIN right away and does not block waiting for input.
 *
 * On any failure, sets client.status = 500 and client.state = WRITING
 * so the normal error-response path takes over.
 */
void ServerManager::startCgi(ClientSession &client,
							  const std::string &scriptPath,
							  const std::string &interpreter,
							  ServerConfig &server) {
	try
	{
		// CgiHandler::start() creates pipes, sets them non-blocking,
		// forks, and execs. Parent's unused pipe ends are already closed inside.
		client.cgi = CgiHandler::start(client.request, scriptPath, interpreter, server);
		client.cgiOutputBuffer.clear();
		client.cgiInputBuffer = client.request.getBody();
		client.cgiInputWritten = 0;

		// Track pipe read-end: epoll fd → client fd → server index
		_cgiReadFdToClient[client.cgi.readFd] = client.fd;
		_cgiClientToServer[client.fd] = client.ownerIndex;

		// Register read-end in epoll — we always want to capture output
		addToEpoll(_epollFd, client.cgi.readFd, EPOLLIN);
		if (!client.cgiInputBuffer.empty())
		{
			// POST: register write-end so epoll tells us when we can push body
			_cgiWriteFdToClient[client.cgi.writeFd] = client.fd;

			struct epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLOUT;
			ev.data.fd = client.cgi.writeFd;
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, client.cgi.writeFd, &ev) < 0)
				throw std::runtime_error("epoll_ctl failed for CGI writeFd");
		}
		else
		{
			// GET (no body): close write-end immediately so child gets EOF on STDIN
			// and does not hang waiting for input that will never arrive.
			close(client.cgi.writeFd);
			client.cgi.writeFd = -1;
		}

		// Client socket stays in its current epoll state (EPOLLIN from reading).
		// We do NOT switch it to EPOLLOUT yet — we wait until CGI output is ready.
		printLog("🚀 CGI launched pid=" + itostr(client.cgi.pid) + " script=" + scriptPath, BGRN);
	}
	catch (const std::exception &e)
	{
		printLog("🚨 CGI launch failed: " + std::string(e.what()), RED);
		cleanupCgi(client);
		client.status = 500;
		client.keepAlive = false;
		client.state = WRITING;
		modClientEpoll(client, EPOLLOUT);
	}
}

/**
 * @brief Writes pending POST body bytes to the CGI process stdin pipe.
 *
 * write() may accept fewer bytes than requested (partial write) because the
 * pipe buffer is finite. We advance cgiInputWritten and return; epoll will
 * fire EPOLLOUT again when the pipe has more space.
 *
 * Once all bytes are written, the write-end is removed from epoll and closed.
 * Closing it delivers EOF to the child's STDIN so the script knows the body
 * is complete and can start processing.
 */
void ServerManager::handleCgiWrite(int clientFd, int serverIndex)
{
	if (serverIndex < 0 || static_cast<size_t>(serverIndex) >= _clients.size())
		return;

	std::map<int, ClientSession>::iterator it = _clients[serverIndex].find(clientFd);
	if (it == _clients[serverIndex].end())
		return;
	ClientSession &client = it->second;

	if (client.cgi.writeFd < 0)
		return;

	size_t  remaining = client.cgiInputBuffer.size() - client.cgiInputWritten;
	ssize_t written = write(client.cgi.writeFd, client.cgiInputBuffer.c_str() + client.cgiInputWritten, remaining);

	if (written > 0)
		client.cgiInputWritten += static_cast<size_t>(written);

	// When every byte has been delivered, close write-end → EOF to child
	if (client.cgiInputWritten >= client.cgiInputBuffer.size())
	{
		removeFromEpoll(_epollFd, client.cgi.writeFd);
		_cgiWriteFdToClient.erase(client.cgi.writeFd);
		close(client.cgi.writeFd);
		client.cgi.writeFd = -1;
		printLog("✅ CGI stdin fully written, closed write-end", BGRN);
	}
}

/**
 * @brief Reads available CGI output bytes and detects script completion.
 *
 * Accumulates data in cgiOutputBuffer across multiple epoll events.
 *
 * The child is considered done when:
 *   - read() returns 0  (pipe closed, i.e. child exited or closed stdout)
 *   - epoll fires EPOLLHUP or EPOLLERR on readFd
 *
 * On completion:
 *   1. waitpid(WNOHANG) reaps the child to avoid zombies.
 *   2. CgiHandler::buildResponse() converts the raw CGI output to HTTP.
 *   3. The client socket is switched to EPOLLOUT so the response is sent.
 *
 * @param clientFd    The client socket fd that owns this CGI process.
 * @param serverIndex Index into _clients and _servers for this client.
 */
void ServerManager::handleCgiRead(int clientFd, int serverIndex)
{
	if (serverIndex < 0 || static_cast<size_t>(serverIndex) >= _clients.size())
		return;

	std::map<int, ClientSession>::iterator it = _clients[serverIndex].find(clientFd);
	if (it == _clients[serverIndex].end())
		return;

	ClientSession &client = it->second;
	if (client.cgi.readFd < 0)
		return;

	// Read all currently available bytes (non-blocking — stops at EAGAIN)
	char buf[BUFFER_SIZE];
	bool pipeEof = false;
	ssize_t readBytes;

	while (true)
	{
		readBytes = read(client.cgi.readFd, buf, sizeof(buf));
		if (readBytes > 0)
			client.cgiOutputBuffer.append(buf, static_cast<size_t>(readBytes));
		else {
			pipeEof = true;
			break;
		}
		// I cannot use errno to change behavior
	}

	if (!pipeEof)
		return; // More data may arrive, keep EPOLLIN armed

	// Reap child process — WNOHANG so we never block the event loop
	int status;
	waitpid(client.cgi.pid, &status, WNOHANG);
	client.cgi.pid = -1;

	// Remove read-end from epoll and tracking map, close it
	removeFromEpoll(_epollFd, client.cgi.readFd);
	_cgiReadFdToClient.erase(client.cgi.readFd);
	close(client.cgi.readFd);
	client.cgi.readFd = -1;
	_cgiClientToServer.erase(client.fd);

	// Build HTTP response from raw CGI output
	client.writeBuffer = CgiHandler::buildResponse(client.cgiOutputBuffer, client.version, client.keepAlive);
	client.totalSent = 0;
	client.cgiOutputBuffer.clear();

	// Switch client socket to write-ready so the response is sent
	client.state = WRITING;
	modClientEpoll(client, EPOLLOUT);
	printLog("✅ CGI response ready, switching to WRITING fd=" + itostr(client.fd), BGRN);
}

/**
 * @brief Releases all CGI resources attached to a client session.
 *
 * Safe to call even if the CGI was never fully launched (fd == -1 guards).
 * Must be called before erasing the ClientSession from _clients[].
 */
void ServerManager::cleanupCgi(ClientSession &client)
{
	if (client.cgi.pid > 0) {
		kill(client.cgi.pid, SIGKILL);
		waitpid(client.cgi.pid, NULL, 0); // blocking wait after SIGKILL is safe
		client.cgi.pid = -1;
	}
	if (client.cgi.writeFd >= 0) {
		removeFromEpoll(_epollFd, client.cgi.writeFd);
		_cgiWriteFdToClient.erase(client.cgi.writeFd);
		close(client.cgi.writeFd);
		client.cgi.writeFd = -1;
	}
	if (client.cgi.readFd >= 0) {
		removeFromEpoll(_epollFd, client.cgi.readFd);
		_cgiReadFdToClient.erase(client.cgi.readFd);
		close(client.cgi.readFd);
		client.cgi.readFd = -1;
	}

	_cgiClientToServer.erase(client.fd);
	client.cgiOutputBuffer.clear();
	client.cgiInputBuffer.clear();
	client.cgiInputWritten = 0;
}