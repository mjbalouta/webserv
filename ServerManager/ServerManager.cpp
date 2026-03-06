#include "ServerManager.hpp"
#include "Server.hpp"
#include "Config.hpp"
#include "Connection.hpp"
#include "../Utils.hpp"

/**
 * @brief Constructs ServerManager and initializes all server sockets
 * @param argv Command line arguments (argv[1] should be config file path)
 * @throw std::runtime_error if config parsing, validation, or socket setup fails
 */
ServerManager::ServerManager(char **argv) : _epollFd(-1) {
	std::ifstream file(argv[1]);
	Config config;
	_configs = config.getConfig();
	printMessage("🛠️ Done parsing config file ", CYAN);

	try
	{
		parseConfigServers();
		printMessage("🚧 Setting up servers...", GOLD);
		_epollFd = epoll_create(1);  // Create epoll instance, Parameter 1 is a hint about how many file descriptors you'll add (ignored on modern Linux, kept for compatibility)
		if (_epollFd < 0)
			throw std::runtime_error("Failed to create epoll instance");
		_connections.resize(_servers.size());
		createServerSockets();
	}
	catch(const std::exception& e)
	{
		cleanupConnections();
		cleanupSockets();
		_connections.clear();
		_servers.clear();
		if (_epollFd >= 0) {
			close(_epollFd);
			_epollFd = -1;
		}
		throw;
	}
}

/**
 * @brief 
 *  Creates and configures TCP server sockets for each configured server.
 *  For each server, this function: Create non-blocking TCP sockets, Enable quick restart on same port
 *  Bind socket to IP address and port, Mark the socket as listening and Register the socket with epoll
 * @throw std::runtime_error if socket creation, binding, or configuration fails
 */
void ServerManager::createServerSockets(){
	printMessage("🔧 Creating server sockets...", BBLU);
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::string serverInfo = _servers[i].getServerIp() + ":" + itostr(_servers[i].getPort());
		try
		{
			int server_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET - Address Family: IPv4. SOCK_STREAM - Socket Type: TCP.  0 - Protocol: Let the OS choose the protocol
			if (server_fd < 0)
				throw std::runtime_error("Failed to create socket for " + serverInfo);
			_servers[i].setServerFd(server_fd);
			
			int opt = 1;
			if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) //server_fd - The socket file descriptor to configure. SOL_SOCKET - Level: Socket-level options (vs IPPROTO_TCP for TCP-level). SO_REUSEADDR - Option: Allow reusing local address/port. &opt - Pointer to the value, 1 means "enable"). sizeof(opt) - Size of the value
				throw std::runtime_error("Failed to set SO_REUSEADDR for " + serverInfo);
			
			struct sockaddr_in addr; //Creates a structure to hold IPv4 address information sockaddr_in = socket address for IPv4
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(_servers[i].getServerIp().c_str()); //Converts IP string (e.g., "127.0.0.1") to binary format, inet_addr() = internet address conversion
			addr.sin_port = htons(_servers[i].getPort()); //Converts port number to network byte order (big-endian). htons() = host to network short (converts integer to network format)
			
			if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) //After bind: Socket is attached to 127.0.0.1:8080 (or your configured IP:port)
				throw std::runtime_error("Failed to bind socket " + serverInfo + " (address already in use?)");
			
			if (listen(server_fd, SOMAXCONN) < 0) //After listen: Socket is in "listening" state, ready for accept()
				throw std::runtime_error("Failed to listen on " + serverInfo);
			
			setnon_blocking(server_fd); //sets a socket to non-blocking mode, meaning socket operations won't wait (block) for completion.
			
			addToEpoll(_epollFd, server_fd, EPOLLIN); //EPOLLIN - Monitor for incoming data/connections (read readiness)
			printMessage("✅ Server running at 🌐 http://" + serverInfo, BGRN);
		}
		catch (const std::exception& e)
		{
			// If socket was created but setup failed, close it
			int fd = _servers[i].getServerfd();
			if (fd >= 0) {
				close(fd);
				_servers[i].setServerFd(-1);
			}
			throw;
		}
	}
	printMessage("🎉 All servers are up and running smoothly! 🚀", BMAG);
}

/**
 * @brief Parses configuration and creates Server objects with validation
 * @throw std::runtime_error if validation fails (invalid IP, port, or duplicate binding)
 */
void ServerManager::parseConfigServers() {
	for (size_t i = 0; i < _configs.size(); i++) {
		for(size_t j = 0; j < _configs[i].getPort().size(); j++) {
			Server server;
			int port = std::atoi(_configs[i].getPort()[j].c_str());
			// Validate port range
			validatePort(port, i);
			// Configure server
			server.setPort(port);
			server.setName(_configs[i].getServerName()[j]);
			server.setIp(_configs[i].getHost()[j]);
			server.setMaxBody(std::stol(_configs[i].getClientMaxBodySize()[j]));
			server.setRoot(_configs[i].getDefaultRoot()[j]);
			server.setIndex(i);
			_servers.push_back(server);
		}
	}
}

/**
 * @brief Accepts a new client connection and registers it with epoll
 * @param fd Server socket file descriptor (listening socket)
 * @param serverIndex Server index for storing the connection in _connections map
 * @return true if connection was successfully accepted and registered, false otherwise
 */
bool ServerManager::createConnection(int fd, int serverIndex)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	// accept(fd, addr, addrlen) - Accepts an incoming connection on listening socket 'fd'
	// Returns a new socket fd for communicating with the client, or -1 on error
	// client_addr gets filled with the client's address info (IP, port)
	// client_len is both input (max size to write) and output (actual size written)
	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0)
		return (printMessage("❌ Failed to accept connection", RED), false);
	setnon_blocking(client_fd);
	addToEpoll(_epollFd ,client_fd, EPOLLIN);
	_connections[serverIndex][client_fd] = Connection(client_fd);
	_connections[serverIndex][client_fd]._state = READING;
	printMessage("🔗 New connection accepted", GRN);
	return true;
}

/**
 * @brief Sets a file descriptor to non-blocking mode using fcntl
 * @param fd The file descriptor to configure
 * @throw std::runtime_error if F_GETFL or F_SETFL fcntl operations fail
 * @details 
 *   - F_GETFL: Retrieves current flags set on the file descriptor
 *   - O_NONBLOCK: Flag that makes socket operations non-blocking
 *   - F_SETFL: Sets new flags (ORed with existing flags) on the file descriptor
 *   Non-blocking sockets allow multiplexing many connections without threads
 */
void setnon_blocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		throw std::runtime_error("fcntl F_GETFL failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl F_SETFL failed");
}

/**
 * @brief Validates port number range
 * @param port Port number to validate
 * @param serverIndex Server index for error reporting
 * @throw std::runtime_error if port is out of valid range (1-65535)
 */
void ServerManager::validatePort(int port, int serverIndex) const {
	if (port < 1 || port > 65535) {
		throw std::runtime_error("Invalid port " + itostr(port) + " in server config " + itostr(serverIndex) + " (must be 1-65535)");
	}
}

/**
 * @brief Closes all open server socket file descriptors
 * @details Iterates through all server sockets stored in _servers vector,
 *   closes each valid file descriptor (fd >= 0), and resets it to -1
 *   to prevent double-close errors in destructor
 */
void ServerManager::cleanupSockets() {
	for (size_t i = 0; i < _servers.size(); i++) {
		int fd = _servers[i].getServerfd();
		if (fd >= 0) {
			close(fd);
			_servers[i].setServerFd(-1);
		}
	}
}
/**
 * @brief Closes all open client connection file descriptors
 * @throw std::runtime_error if epoll_ctl fails
 */
void ServerManager::cleanupConnections() {
	for (size_t i = 0; i < _connections.size(); i++) {
		std::map<int, Connection>::iterator it = _connections[i].begin();
		while (it != _connections[i].end()) {
			int fd = it->first;
			++it;
			closeConnection(i, fd);
		}
	}
}

void ServerManager::closeConnection(int serverIndex, int fd)
{
	if (serverIndex < 0 || static_cast<size_t>(serverIndex) >= _connections.size() || fd < 0)
		return;

	std::map<int, Connection> &serverConnections = _connections[serverIndex];
	std::map<int, Connection>::iterator it = serverConnections.find(fd);
	if (it == serverConnections.end())
		return;

	removeFromEpoll(_epollFd, fd);
	it->second.closeConnection();
	serverConnections.erase(it);
	printMessage("Closing connection: " + itostr(fd), MAG);
}

ServerManager::~ServerManager(){
	cleanupConnections();
	for (size_t i = 0; i < _servers.size(); i++) {
		int fd = _servers[i].getServerfd();
		if (fd >= 0) {
			close(fd);
			_servers[i].setServerFd(-1);
		}
	}
	_connections.clear();
	_servers.clear();
	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}
	_configs.clear();
	printMessage("👋 BYE BYE 🔒 Server shut down", CYAN);
}