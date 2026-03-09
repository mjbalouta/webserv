#pragma once

#include "../Includes.hpp"

class Server;

class ServerManager {
	private:
		int _epollFd
		std::vector<Server *> servers;
		std::vector<Config> configs;
	public :
		ServerManager(char **argv);
		~ServerManager();
		
		void createServerSockets();
};
