#include "ServerManager.hpp"

/**
 * @brief Creates a default disconnected client session.
 */
ServerManager::ClientSession::ClientSession()
	: fd(-1), ownerIndex(-1), state(IDLE), lastActive(time(NULL)), responseStr(""),
	  readBuffer(""), writeBuffer(""), totalSent(0), totalReceived(0),
	  method(NONE), status(200), path(""), contentLength(0),
	  keepAlive(true), isRedirection(false), headersSent(false) {}

/**
 * @brief Creates a client session associated with an accepted socket fd.
 * @param clientFd Accepted client descriptor.
 */
ServerManager::ClientSession::ClientSession(int clientFd)
	: fd(clientFd), ownerIndex(-1), state(IDLE), lastActive(time(NULL)), responseStr(""),
	  readBuffer(""), writeBuffer(""), totalSent(0), totalReceived(0),
	  method(NONE), status(200), path(""), contentLength(0),
	  keepAlive(true), isRedirection(false), headersSent(false) {}

/**
 * @brief Initializes configuration, expands servers, and boots epoll listeners.
 * @param argv Program arguments where `argv[1]` is the config path.
 * @throw std::runtime_error On epoll/socket initialization failures.
 */
ServerManager::ServerManager(char **argv) : _epollFd(-1)
{
	ConfigParser config;
	config.parse(argv[1]);
	_servers = config.getServers();
	printLog("🛠️  Done parsing config file ", CYAN);

	try
	{
		printLog("🚧 Setting up servers...", GOLD);
		_epollFd = epoll_create(1); // Creates an epoll instance and returns its fd; the argument is ignored on modern Linux and kept for compatibility.
		if (_epollFd < 0)
			throw std::runtime_error("Failed to create epoll instance");
		_clients.resize(_servers.size());
		setupListeningSockets();
	}
	catch (const std::exception&)
	{
		cleanupClients();
		cleanupSockets();
		_clients.clear();
		_clientFdToServer.clear();
		_listenerFdToServer.clear();
		_servers.clear();
		if (_epollFd >= 0)
		{
			close(_epollFd);
			_epollFd = -1;
		}
		throw;
	}
}
