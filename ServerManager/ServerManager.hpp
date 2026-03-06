#pragma once

#include "../Includes.hpp"
#include "../Config/Config.hpp"
#include "Connection.hpp"

class Server;

class ServerManager {
	private:
		int _epollFd;
		std::vector<Server> _servers;
		std::vector<Config> _configs;
		std::vector<std::map<int, Connection>> _connections;
		
		void parseConfigServers();
		void createServerSockets();
		bool createConnection(int fd, int serverIndex);
		void validatePort(int port, int serverIndex) const;
		void cleanupSockets();
		void closeConnection(int serverIndex, int fd);
		void cleanupConnections();
		void addToEpoll(int fd, uint32_t events);
		void removeFromEpoll(int fd);
	public :
		ServerManager(char **argv);
		~ServerManager();
};
