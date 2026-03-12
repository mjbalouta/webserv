#include "ServerManager.hpp"

/**
 * @brief Expands parsed config data into runtime server endpoints.
 */
void ServerManager::parseConfigServers()
{
	for (size_t i = 0; i < _configs.size(); i++)
	{
		for (size_t j = 0; j < _configs[i].getPort().size(); j++)
		{
			Server server;
			int port = std::atoi(_configs[i].getPort()[j].c_str());
			server.setPort(port);
			server.setName(_configs[i].getServerName()[j]);
			server.setIp(_configs[i].getHost()[j]);
			server.setMaxBody(strToLong(_configs[i].getClientMaxBodySize()[j]));
			server.setRoot(_configs[i].getDefaultRoot()[j]);
			server.setIndex(i);
			_servers.push_back(server);
		}
	}
}

/**
 * @brief Creates one listening socket with bind/listen and non-blocking mode.
 * @param server Endpoint configuration source.
 * @param serverInfo Human-readable ip:port for error messages.
 * @return Listening socket fd.
 */
int ServerManager::buildListeningSocket(const Server &server, const std::string &serverInfo)
{
	int serverFd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4 address family. SOCK_STREAM: TCP socket type. 0: let OS select protocol.
	if (serverFd < 0)
		throw std::runtime_error("Failed to create socket for " + serverInfo);

	int opt = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // serverFd: target socket. SOL_SOCKET: socket-level option. SO_REUSEADDR: allow local addr/port reuse. &opt=1 enables it.
	{
		close(serverFd);
		throw std::runtime_error("Failed to set SO_REUSEADDR for " + serverInfo);
	}

	struct sockaddr_in addr; // IPv4 socket address structure.
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(server.getServerIp().c_str()); // Converts dotted IPv4 string (e.g. "127.0.0.1") to binary network address.
	addr.sin_port = htons(server.getPort()); // Host-to-network short: converts port to big-endian network byte order.

	if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) // Associates socket with configured IP:port.
	{
		close(serverFd);
		throw std::runtime_error("Failed to bind socket " + serverInfo + " (address already in use?)");
	}

	if (listen(serverFd, SOMAXCONN) < 0) // Puts socket in listening state so incoming connections can be accepted.
	{
		close(serverFd);
		throw std::runtime_error("Failed to listen on " + serverInfo);
	}

	setNonBlockingFd(serverFd); // Non-blocking mode: socket operations return immediately instead of waiting.
	return serverFd;
}

/**
	* @brief Registers a listening fd in epoll for read-ready events.
 * @param fd Listening socket fd.
 * @param serverIndex Owning server index.
 */
void ServerManager::addListenerToEpoll(int fd, int serverIndex)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) // Adds listening fd to epoll interest list; EPOLLIN notifies pending incoming connections.
		throw std::runtime_error("Failed to add fd " + itostr(fd) + " to epoll");
	_listenerFdToServer[fd] = serverIndex;
}

/**
	* @brief Registers a client fd in epoll for read-ready events.
 * @param client Client session to register.
 */
void ServerManager::addClientToEpoll(ClientSession &client)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = client.fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, client.fd, &ev) < 0)
		throw std::runtime_error("Failed to add fd " + itostr(client.fd) + " to epoll");
}

/**
	* @brief Updates epoll interest mask for a client.
 * @param client Target client session.
 * @param events New event mask.
 */
void ServerManager::modClientEpoll(const ClientSession &client, uint32_t events)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = client.fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, client.fd, &ev) < 0)
		throw std::runtime_error("Failed to modify fd " + itostr(client.fd) + " in epoll");
}

/**
 * @brief Creates and registers all configured listening sockets.
 */
void ServerManager::setupListeningSockets()
{
	printLog("🔧 Creating server sockets...", BBLU);
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::string serverInfo = _servers[i].getServerIp() + ":" + itostr(_servers[i].getPort());
		try
		{
			int server_fd = buildListeningSocket(_servers[i], serverInfo);
			_servers[i].setServerFd(server_fd);
			addListenerToEpoll(server_fd, static_cast<int>(i));
			printLog("✅ Server running at 🌐 http://" + serverInfo, BGRN);
		}
		catch (const std::exception&)
		{
			int fd = _servers[i].getServerfd();
			if (fd >= 0)
			{
				_listenerFdToServer.erase(fd);
				close(fd);
				_servers[i].setServerFd(-1);
			}
			throw;
		}
	}
	printLog("🎉 All servers are up and running smoothly! 🚀", BMAG);
}
