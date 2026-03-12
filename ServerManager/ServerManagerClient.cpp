#include "ServerManager.hpp"

/**
 * @brief Accepts one pending connection and creates a tracked client session.
 * @param fd Listening socket fd.
 * @param serverIndex Owning server index.
 * @return true when a client was accepted, false when no pending client/error.
 */
bool ServerManager::acceptClientConnection(int fd, int serverIndex)
{
	// Storage for the peer address returned by accept()
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	// accept() removes one pending connection from the listening socket queue
	// and returns a brand-new connected client socket fd.
	int client_fd = accept(fd, (struct sockaddr *)&clientAddr, &clientLen);
	if (client_fd < 0)
	{
		// EAGAIN/EWOULDBLOCK: No pending connections (non-blocking socket behavior). Retry on next epoll event.
		// EINTR: System call was interrupted by a signal. Safe to retry.
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		return (printLog("🚨 Accept failed on listening socket", RED), false);
	}

	// Accepted sockets must also be non-blocking, otherwise one slow client could block the whole event loop during recv()/send().
	setNonBlockingFd(client_fd);
	_clients[serverIndex][client_fd] = ClientSession(client_fd);
	// Remember which server block accepted this client.
	_clients[serverIndex][client_fd].ownerIndex = serverIndex;
	_clientFdToServer[client_fd] = serverIndex;
	// Register the new client in epoll to watch for readable incoming data.
	addClientToEpoll(_clients[serverIndex][client_fd]);
	// New connections always start in READING state, waiting for the first request.
	_clients[serverIndex][client_fd].state = READING;
	printLog("👤 New client fd=" + itostr(client_fd), BGRN);
	return true;
}

/**
 * @brief Closes all listening sockets currently registered in `_servers`.
 */
void ServerManager::cleanupSockets()
{
	for (std::map<int, int>::iterator it = _listenerFdToServer.begin(); it != _listenerFdToServer.end(); ++it)
	{
		int fd = it->first;
		if (fd >= 0)
			close(fd);
	}
	// Clear the mapping now that no listening sockets remain open.
	_listenerFdToServer.clear();

	// Reset the stored fd inside each ServerConfig so runtime state matches reality.
	for (size_t i = 0; i < _servers.size(); ++i)
		_servers[i].setFd(-1);
}

/**
 * @brief Closes all active client sessions across all server groups.
 */
void ServerManager::cleanupClients()
{
	// Iterate over each server's client map because clients are grouped by owner server.
	for (size_t i = 0; i < _clients.size(); i++)
	{
		std::map<int, ClientSession>::iterator it = _clients[i].begin();
		while (it != _clients[i].end())
		{
			int fd = it->first;
			++it;
			closeClient(static_cast<int>(i), fd);
		}
	}
}

/**
 * @brief Removes one client from epoll and internal tracking map.
 * @param serverIndex Owning server index.
 * @param fd Client socket fd.
 */
void ServerManager::closeClient(int serverIndex, int fd)
{
	// Ignore impossible or already-invalid inputs.
	if (serverIndex < 0 || static_cast<size_t>(serverIndex) >= _clients.size() || fd < 0)
		return;

	// Look up the client in the owning server's client map.
	std::map<int, ClientSession> &serverClients = _clients[serverIndex];
	std::map<int, ClientSession>::iterator it = serverClients.find(fd);
	if (it == serverClients.end())
		return;

	// Remove the fd from epoll first so the kernel stops sending readiness events for a socket that is about to disappear.
	removeFromEpoll(_epollFd, fd);
	// Close the socket itself and mark the session state as CLOSING.
	closeClientSocket(it->second);
	// Remove the reverse lookup entry fd -> serverIndex.
	_clientFdToServer.erase(fd);
	// Finally erase the session object from the server's client map.
	serverClients.erase(it);
	printLog("Closed client fd=" + itostr(fd), MAG);
}

/**
 * @brief Closes client fd and marks its state as `CLOSING`.
 * @param client Session to close.
 */
void ServerManager::closeClientSocket(ClientSession &client)
{
	// Only close real open sockets; fd == -1 means already closed/reset.
	if (client.fd >= 0)
	{
		close(client.fd);
		client.fd = -1;
	}
	client.state = CLOSING;
}

/**
 * @brief Releases all runtime resources owned by server manager.
 */
ServerManager::~ServerManager()
{
	cleanupClients();
	cleanupSockets();
	_clients.clear();
	_clientFdToServer.clear();
	_servers.clear();
	if (_epollFd >= 0)
	{
		close(_epollFd);
		_epollFd = -1;
	}
	//_configs.clear();
	printLog("👋 BYE BYE 🔒 Server shut down", CYAN);
}
