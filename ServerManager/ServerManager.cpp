#include "ServerManager.hpp"

/**
 * @brief Creates a default disconnected client session.
 */
ServerManager::ClientSession::ClientSession()
	: fd(-1), ownerIndex(-1), state(IDLE), lastActive(time(NULL)), responseStr(""),
	  readBuffer(""), writeBuffer(""), totalSent(0), totalReceived(0),
	  method(NONE), status(200), path(""), version("HTTP/1.1"), contentLength(0),
	  keepAlive(true), isRedirection(false), headersSent(false), chunkedDecoded(false),
	  chunkedBodyStart(0), chunkedCursor(0), chunkedDecodedBody(""),
	  ioFailures(0), cgiInputWritten(0) {}

/**
 * @brief Creates a client session associated with an accepted socket fd.
 * @param clientFd Accepted client descriptor.
 */
ServerManager::ClientSession::ClientSession(int clientFd)
	: fd(clientFd), ownerIndex(-1), state(IDLE), lastActive(time(NULL)), responseStr(""),
	  readBuffer(""), writeBuffer(""), totalSent(0), totalReceived(0),
	  method(NONE), status(200), path(""), version("HTTP/1.1"), contentLength(0),
	  keepAlive(true), isRedirection(false), headersSent(false), chunkedDecoded(false),
	  chunkedBodyStart(0), chunkedCursor(0), chunkedDecodedBody(""),
	  ioFailures(0), cgiInputWritten(0) {}

/**
 * @brief Initializes configuration, expands servers, and boots epoll listeners.
 * @param argv Program arguments where `argv[1]` is the config path.
 * @throw std::runtime_error On epoll/socket initialization failures.
 */
ServerManager::ServerManager(char **argv) : _epollFd(-1)
{
	try
	{
		ConfigParser config;
		config.parse(argv[1]);
		_servers = config.getServers();
		printLog("⚙️  Loading configuration...", BCYAN);
		_epollFd = epoll_create(1);
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