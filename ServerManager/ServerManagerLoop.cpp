#include "ServerManager.hpp"

/**
 * @brief Closes connections that exceeded keep-alive timeout.
 * @param now Current epoch time.
 */
void ServerManager::closeIdleClients(time_t now)
{
	for (size_t serverIndex = 0; serverIndex < _clients.size(); ++serverIndex)
	{
		std::map<int, ClientSession>::iterator clientIt = _clients[serverIndex].begin();
		while (clientIt != _clients[serverIndex].end())
		{
			int fd = clientIt->first;
			ClientSession &client = clientIt->second;
			++clientIt;

			if (difftime(now, client.lastActive) > KEEP_ALIVE_TIMEOUT)
			{
				printLog("⏳ Closing idle connection: " + itostr(fd), BYEL);
				closeClient(static_cast<int>(serverIndex), fd);
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
	int fd = event.data.fd;

	std::map<int, int>::iterator listenerIt = _listenerFdToServer.find(fd);
	if (listenerIt != _listenerFdToServer.end())
	{
		int listeningServerIndex = listenerIt->second;
		if (listeningServerIndex < 0 || static_cast<size_t>(listeningServerIndex) >= _servers.size())
			return;
		int listenFd = _servers[static_cast<size_t>(listeningServerIndex)].getFd();
		while (acceptClientConnection(listenFd, listeningServerIndex))
			;
		return;
	}

	std::map<int, int>::iterator ownerIt = _clientFdToServer.find(fd);
	if (ownerIt == _clientFdToServer.end())
		return;

	int ownerIndex = ownerIt->second;
	if (ownerIndex < 0 || static_cast<size_t>(ownerIndex) >= _clients.size())
		return;

	std::map<int, ClientSession>::iterator clientIt = _clients[ownerIndex].find(fd);
	if (clientIt == _clients[ownerIndex].end())
		return;

	ClientSession &client = clientIt->second;
	if (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		client.state = CLOSING;
	else
	{
		if (static_cast<size_t>(ownerIndex) >= _servers.size())
			client.state = CLOSING;
		else
			handleClientRequest(client, static_cast<size_t>(_servers[ownerIndex].getMaxBodySize()), _epollFd);
	}

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
		int ready = epoll_wait(_epollFd, events, MAX_EVENTS, 1000);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait failed");
		}

		closeIdleClients(time(NULL));
		for (int index = 0; index < ready; ++index)
			handleReadyEvent(events[index]);
	}
}
