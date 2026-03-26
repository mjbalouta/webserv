#pragma once

#include "../config/ServerConfig.hpp"
#include "Request.hpp"
#include "../Utils.hpp"
#include "../config/parser/ConfigParser.hpp"
#include "../Includes.hpp"

class ServerConfig;

class ServerManager {
	private:
		enum State {
			READING,
			WRITING,
			CLOSING,
			PROCESSING,
			IDLE
		};

		struct ClientSession {
			int fd;
			int ownerIndex;
			State state;
			time_t lastActive;
			std::string responseStr;
			std::string readBuffer;
			std::string writeBuffer;
			size_t totalSent;
			size_t totalReceived;
			Method method;
			int status;
			std::string path;
			std::string version;
			size_t contentLength;
			bool keepAlive;
			bool isRedirection;
			bool headersSent;
			int ioFailures;  // used to detect errors without errno
			Request request;
			ClientSession();
			explicit ClientSession(int clientFd);
		};

		int _epollFd;
		std::vector<ServerConfig> _servers;
		//std::vector<Config> _configs;
		std::vector<std::map<int, ClientSession> > _clients;
		std::map<int, int> _listenerFdToServer;
		std::map<int, int> _clientFdToServer;

		int buildListeningSocket(const ServerConfig &server, int port, const std::string &serverInfo);
		bool acceptClientConnection(int fd, int serverIndex);
		void setupListeningSockets();
		void addListenerToEpoll(int fd, int serverIndex);
		void addClientToEpoll(ClientSession &client);
		void modClientEpoll(const ClientSession &client, uint32_t events);
		void handleClientRequest(ClientSession &client, ServerConfig &server);
		void readClientRequest(ClientSession &client, size_t maxUploadSize);
		void parseClientRequest(ClientSession &client, ServerConfig &server);
		void processClientRequest(ClientSession &client, Request &request, ServerConfig &server);
		void sendClientResponse(ClientSession &client, ServerConfig &server);
		void handleReadyEvent(const epoll_event &event);
		void cleanupSockets();
		void cleanupClients();
		void closeClient(int serverIndex, int fd);
		void closeClientSocket(ClientSession &client);
		void closeIdleClients(time_t now);
		void decodeChunked(ClientSession &client, size_t maxUploadSize);
		int parseTransferEncodingHeader(const std::string &headersLower, bool &hasTransferEncoding, bool &isChunkedOnly);

	public :
		ServerManager(char **argv);
		~ServerManager();
		void runEventLoop();
};