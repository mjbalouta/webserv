#include "ServerManager.hpp"
#include "../ResponseBuilder.hpp"

/**
 * @brief Closes connections that exceeded keep-alive timeout.
 * @param now Current epoch time.
 */
void ServerManager::closeIdleClients(time_t now)
{
	// Clients are grouped by owning server, so scan each server's client map.
	for (size_t serverIndex = 0; serverIndex < _clients.size(); ++serverIndex)
	{
		std::map<int, ClientSession>::iterator clientIt = _clients[serverIndex].begin();
		while (clientIt != _clients[serverIndex].end())
		{
			int fd = clientIt->first;
			ClientSession &client = clientIt->second;
			++clientIt;

			// difftime(now, lastActive) returns how many seconds the client has been idle.
			if (difftime(now, client.lastActive) > KEEP_ALIVE_TIMEOUT)
			{
				printLog("⏳ Closing idle connection: " + itostr(fd), BYEL);
				closeClient(static_cast<int>(serverIndex), fd);
			}
			// CGI timeout: if a child process has been running > CGI_TIMEOUT seconds, kill it
			if (client.cgi.pid > 0 && difftime(now, client.cgi.startTime) > CGI_TIMEOUT)
			{
				printLog("⏳ CGI timeout pid=" + itostr(client.cgi.pid) + " fd=" + itostr(fd), BYEL);
				cleanupCgi(client);
				client.status = 504;
				client.keepAlive = false;
				// Build a 504 error response and switch to writing
				Request errorReq;
				errorReq.setStatus(504);
				errorReq.setVersion(client.version);
				ConfigResolved config(errorReq, _servers[serverIndex]);
				ResponseBuilder rb;
				client.writeBuffer = rb.returnGenericErrorResponse(504, errorReq, config);
				client.totalSent = 0;
				client.state = WRITING;
				modClientEpoll(client, EPOLLOUT);
			}
		}
	}
}

/**
 * @brief Dispatches one epoll event to listener accept path or client path.
 * @param event Ready epoll event.
 */
void ServerManager::handleReadyEvent(const epoll_event &event)
{
	// epoll tells us which fd became ready through event.data.fd.
	int fd = event.data.fd;

	//CGIII
	// Check if this fd is a CGI read pipe (pipe_out[0])
	std::map<int, int>::iterator cgiReadIt = _cgiReadFdToClient.find(fd);
	if (cgiReadIt != _cgiReadFdToClient.end())
	{
		int clientFd = cgiReadIt->second;
		std::map<int, int>::iterator ownerIt = _cgiClientToServer.find(clientFd);
		int serverIndex = (ownerIt != _cgiClientToServer.end()) ? ownerIt->second : -1;
		handleCgiRead(clientFd, serverIndex);
		return;
	}

	// Check if this fd is a CGI write pipe (pipe_in[1])
	std::map<int, int>::iterator cgiWriteIt = _cgiWriteFdToClient.find(fd);
	if (cgiWriteIt != _cgiWriteFdToClient.end())
	{
		int clientFd = cgiWriteIt->second;
		std::map<int, int>::iterator ownerIt = _cgiClientToServer.find(clientFd);
		int serverIndex = (ownerIt != _cgiClientToServer.end()) ? ownerIt->second : -1;
		handleCgiWrite(clientFd, serverIndex);
		return;
	}

	//NORMAL
	// First check whether this fd is one of the listening sockets.
	// If yes, the event means at least one new incoming connection is waiting.
	std::map<int, int>::iterator listenerIt = _listenerFdToServer.find(fd);
	if (listenerIt != _listenerFdToServer.end())
	{
		int listeningServerIndex = listenerIt->second;
		if (listeningServerIndex < 0 || static_cast<size_t>(listeningServerIndex) >= _servers.size())
			return;
		// Keep calling acceptClientConnection() until it returns false.
		while (acceptClientConnection(fd, listeningServerIndex))
			;
		return;
	}

	// Otherwise this should be a connected client fd
	std::map<int, int>::iterator ownerIt = _clientFdToServer.find(fd);
	if (ownerIt == _clientFdToServer.end())
		return;

	int ownerIndex = ownerIt->second;
	if (ownerIndex < 0 || static_cast<size_t>(ownerIndex) >= _clients.size())
		return;

	// Find the actual client session object that belongs to this fd.
	std::map<int, ClientSession>::iterator clientIt = _clients[ownerIndex].find(fd);
	if (clientIt == _clients[ownerIndex].end())
		return;

	ClientSession &client = clientIt->second;
	// EPOLLERR: the socket entered an error state.
	// EPOLLHUP: the connection was fully hung up / disconnected.
	// EPOLLRDHUP: the peer closed its write side, so no more data will arrive.
	// In all three cases, this client is no longer safe to keep active.
	if (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		client.state = CLOSING;
	else
	{
		// Extra safety check before dereferencing the owner server.
		if (static_cast<size_t>(ownerIndex) >= _servers.size())
			client.state = CLOSING;
		else
		{
			ServerConfig &ownerServer = _servers[static_cast<size_t>(ownerIndex)];
			handleClientRequest(client, ownerServer);
		}
	}

	// If request handling decided the session must die, close it now.
	if (client.state == CLOSING)
		closeClient(ownerIndex, fd);
}

/**
 * @brief Runs the main blocking epoll loop.
 */
void ServerManager::runEventLoop()
{
	struct epoll_event events[MAX_EVENTS];

	while (true)
	{
		// epoll_wait() blocks until:
		// - at least one fd becomes ready,
		// - the timeout expires (1000 ms), or
		// - a signal interrupts the system call.
		int ready = epoll_wait(_epollFd, events, MAX_EVENTS, 1000);
		if (ready < 0)
		{
			// EINTR: System call was interrupted by a signal. Safe to retry epoll_wait immediately.
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait failed");
		}

		closeIdleClients(time(NULL));
		for (int index = 0; index < ready; ++index)
			handleReadyEvent(events[index]);
	}
}
