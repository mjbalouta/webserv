#include "ServerManager.hpp"

/**
 * @brief Resolves a config host token into an IPv4 address for bind().
 * @param host Host token from config (examples: "localhost", "127.0.0.1", "0.0.0.0", "*").
 * @return IPv4 address in network byte order suitable for `sockaddr_in::sin_addr.s_addr`.
 * @throw std::runtime_error if `host` is not a supported IPv4 token.
 */
static in_addr_t resolveBindAddress(const std::string &host)
{
	// Bind on all local network interfaces when host is wildcard/unspecified.
	// INADDR_ANY means "accept connections on any IPv4 address this machine owns".
	if (host.empty() || host == "*" || host == "0.0.0.0")
		return htonl(INADDR_ANY);

	// Bind only on the local loopback interface when config uses "localhost".
	// INADDR_LOOPBACK is the IPv4 loopback address 127.0.0.1.
	// Only clients on the same machine can connect to this address.
	if (host == "localhost")
		return htonl(INADDR_LOOPBACK);

	// Parse dotted IPv4 notation (e.g., "192.168.1.10" or "127.0.0.1").
	// AF_INET tells inet_pton() to parse the text as an IPv4 address.
	// inet_pton() returns:
	// 1 if the text is a valid IPv4 address,
	// 0 if the text is not valid IPv4,
	// -1 if the address family is unsupported.
	struct in_addr parsed;
	if (inet_pton(AF_INET, host.c_str(), &parsed) == 1)
		return parsed.s_addr;

	// Any other token (hostname, IPv6, malformed IPv4) is rejected in this IPv4-only server path.
	throw std::runtime_error("Invalid IPv4 host in config: " + host);
}

/**
 * @brief Creates one listening socket with bind/listen and non-blocking mode.
 * @param server Endpoint configuration source.
 * @param port Port to bind for this listening socket.
 * @param serverInfo Human-readable ip:port for error messages.
 * @return Listening socket fd.
 */
int ServerManager::buildListeningSocket(const ServerConfig &server, int port, const std::string &serverInfo)
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

	// Fill the IPv4 bind address structure used by bind().
	// sockaddr_in contains: family, IPv4 address and port.
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	// AF_INET tells the kernel this is an IPv4 socket address.
	addr.sin_family = AF_INET;
	// Convert the host token from config into a binary IPv4 address.
	addr.sin_addr.s_addr = resolveBindAddress(server.getHost());
	// Convert the numeric port from host byte order to network byte order.
	addr.sin_port = htons(port);

	// Attach the socket to the chosen local IP address and port.
	// After bind(), the socket owns that endpoint.
	if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		close(serverFd);
		throw std::runtime_error("Failed to bind socket " + serverInfo + " (address already in use?)");
	}

	// Turn the bound socket into a listening socket so incoming TCP connections
	// are queued and later accepted with accept().
	if (listen(serverFd, SOMAXCONN) < 0)
	{
		close(serverFd);
		throw std::runtime_error("Failed to listen on " + serverInfo);
	}

	// Listening sockets must be non-blocking so accept() never freezes the entire server.
	setNonBlockingFd(serverFd);
	return serverFd;
}

/**
	* @brief Registers a listening fd in epoll for read-ready events.
 * @param fd Listening socket fd.
 * @param serverIndex Owning server index.
 */
void ServerManager::addListenerToEpoll(int fd, int serverIndex)
{
	// Build the epoll event structure describing what readiness we care about.
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) // Adds listening fd to epoll interest list; EPOLLIN notifies pending incoming connections.
		throw std::runtime_error("Failed to add fd " + itostr(fd) + " to epoll");
	// Keep a reverse lookup so event-loop code can tell which server owns this listener.
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
	// EPOLLIN means "wake me when this fd can be read without blocking".
	// For client sockets, that means incoming request data is available.
	ev.events = EPOLLIN;
	ev.data.fd = client.fd;
	// EPOLL_CTL_ADD tells epoll_ctl() to add this fd as a new watched entry
	// in the epoll interest list (as opposed to modifying or deleting it).
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
	// Rebuild the event mask whenever the client changes phase
	// (for example EPOLLIN while reading, EPOLLOUT while writing).
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = client.fd;
	// EPOLL_CTL_MOD tells epoll_ctl() to modify an fd that is already being watched,
	// replacing its old event mask with the new one stored in `events`.
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, client.fd, &ev) < 0)
		throw std::runtime_error("Failed to modify fd " + itostr(client.fd) + " in epoll");
}

/**
 * @brief Creates and registers all configured listening sockets.
 */
void ServerManager::setupListeningSockets()
{
	printLog("🛰️  Initializing server sockets...", BBLU);
	for (size_t i = 0; i < _servers.size(); i++)
	{
		const std::vector<int> &ports = _servers[i].getPorts();
		if (ports.empty())
			throw std::runtime_error("No ports configured for server " + _servers[i].getHost());

		for (size_t portIndex = 0; portIndex < ports.size(); ++portIndex)
		{
			std::string serverInfo = _servers[i].getHost() + ":" + itostr(ports[portIndex]);
			try
			{
				// Create, configure, bind and listen on this one endpoint.
				int server_fd = buildListeningSocket(_servers[i], ports[portIndex], serverInfo);
				if (_servers[i].getFd() < 0)
					_servers[i].setFd(server_fd);
				addListenerToEpoll(server_fd, static_cast<int>(i));
				printLog("✅ Server listening on 🌐 http://" + serverInfo, BGRN);
			}
			catch (const std::exception&)
			{
				for (std::map<int, int>::iterator it = _listenerFdToServer.begin(); it != _listenerFdToServer.end(); )
				{
					if (it->second == static_cast<int>(i))
					{
						close(it->first);
						_listenerFdToServer.erase(it++);
					}
					else
						++it;
				}
				_servers[i].setFd(-1);
				throw;
			}
		}
	}
	printLog("🚀 Servers ready to accept connections!", BMAG);
}
