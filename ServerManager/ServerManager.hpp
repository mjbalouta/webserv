#pragma once

#include "../config/ServerConfig.hpp"
#include "Request.hpp"
#include "../Utils.hpp"
#include "../config/parser/ConfigParser.hpp"
#include "../Includes.hpp"
#include "../CGI/CGIHandler.hpp"

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
			bool chunkedDecoded;
			std::string resolvedPath;
			// Incremental chunked decoding state — persists across readClientRequest() calls
			size_t chunkedBodyStart;    // offset of body in readBuffer (set once headers arrive)
			size_t chunkedCursor;       // next byte to decode in the raw chunked body
			std::string chunkedDecodedBody; // accumulates fully decoded chunk data
			int ioFailures;  // used to detect errors without errno
			CgiProcess cgi;              // pid + pipe fds for active CGI child
			std::string cgiOutputBuffer; // accumulates raw CGI stdout as epoll delivers it
			std::string cgiInputBuffer;  // copy of request body waiting to be written
			size_t cgiInputWritten; // bytes of cgiInputBuffer already written to pipe
			Request request;
			ClientSession();
			explicit ClientSession(int clientFd);
		};

		int _epollFd;
		std::vector<ServerConfig> _servers;
		std::vector<std::map<int, ClientSession> > _clients;
		std::map<int, int> _listenerFdToServer;
		std::map<int, int> _clientFdToServer;
		std::map<int, int> _cgiReadFdToClient;  // pipe_out[0] fd → client fd
		std::map<int, int> _cgiWriteFdToClient; // pipe_in[1]  fd → client fd
		std::map<int, int> _cgiClientToServer;  // client fd   → server index

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
		void startCgi(ClientSession &client, const std::string &scriptPath, const std::string &interpreter, ServerConfig &server);
		void handleCgiRead(int clientFd, int serverIndex, uint32_t eventFlags);
		void handleCgiWrite(int clientFd, int serverIndex);
		void cleanupCgi(ClientSession &client);
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