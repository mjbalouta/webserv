#pragma once

#include "../Includes.hpp"

class Server;

class ServerManager {
	private:
		int _epollFd;
		std::vector<Server> _servers;
		std::vector<Config> _configs;
		std::vector<std::map<int, Connection> > _connections;

		void parseConfigServers();
		void createServerSockets();
		bool createConnection(int fd, int serverIndex);
		void cleanupSockets();
		void closeConnection(int serverIndex, int fd);
		void cleanupConnections();


	public :
		ServerManager(char **argv);
		~ServerManager();
		void runEventLoop();
};