#pragma once
#include "../Includes.hpp"

class Server {
	private:
		int _serverFd;
		int _port;
		int _index;
		std::string _serverIp;
		std::string _serverName;
		std::string _clientMaxBodySize;
		std::string _defaultRoot;
	
	public:
		Server();
		~Server();
		
		// Setters
		void setServerFd(int fd);
		void setPort(int port);
		void setIp(const std::string& ip);
		void setName(const std::string& name);
		void setMaxBody(const std::string& maxBody);
		void setRoot(const std::string& root);
		void setIndex(int index);
		
		// Getters
		int getServerfd() const;
		int getPort() const;
		std::string getServerIp() const;
		std::string getServerName() const;
};