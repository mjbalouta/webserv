#include "ServerManager.hpp"

/**
 * @brief Accepts one pending connection and creates a tracked client session.
 * @param fd Listening socket fd.
 * @param serverIndex Owning server index.
 * @return true when a client was accepted, false when no pending client/error.
 */
bool ServerManager::acceptClientConnection(int fd, int serverIndex)
{
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int client_fd = accept(fd, (struct sockaddr *)&clientAddr, &clientLen); // accept(listenFd, addrOut, lenInOut): creates a new client socket fd from the listening fd, writes peer address into clientAddr, and updates clientLen with actual size.
	if (client_fd < 0)
	{
		// EAGAIN/EWOULDBLOCK: No pending connections (non-blocking socket behavior). Retry on next epoll event.
		// EINTR: System call was interrupted by a signal. Safe to retry.
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		return (printLog("🟥 Accept failed on listening socket", RED), false);
	}

	setNonBlockingFd(client_fd);
	_clients[serverIndex][client_fd] = ClientSession(client_fd);
	_clients[serverIndex][client_fd].ownerIndex = serverIndex;
	_clientFdToServer[client_fd] = serverIndex;
	addClientToEpoll(_clients[serverIndex][client_fd]);
	_clients[serverIndex][client_fd].state = READING;
	printLog("🔗 Client accepted fd=" + itostr(client_fd), GRN);
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
	_listenerFdToServer.clear();

	for (size_t i = 0; i < _servers.size(); ++i)
		_servers[i].setFd(-1);
}

/**
 * @brief Closes all active client sessions across all server groups.
 */
void ServerManager::cleanupClients()
{
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
	if (serverIndex < 0 || static_cast<size_t>(serverIndex) >= _clients.size() || fd < 0)
		return;

	std::map<int, ClientSession> &serverClients = _clients[serverIndex];
	std::map<int, ClientSession>::iterator it = serverClients.find(fd);
	if (it == serverClients.end())
		return;

	removeFromEpoll(_epollFd, fd);
	closeClientSocket(it->second);
	_clientFdToServer.erase(fd);
	serverClients.erase(it);
	printLog("Closed client fd=" + itostr(fd), MAG);
}

/**
 * @brief Closes client fd and marks its state as `CLOSING`.
 * @param client Session to close.
 */
void ServerManager::closeClientSocket(ClientSession &client)
{
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
