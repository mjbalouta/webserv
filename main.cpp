#include "Request.hpp"
#include "Exception.hpp"
#include "Utils.hpp"
#include "Includes.hpp"

// Minimal HTTP server - Person 1 responsibilities only
// Transport and protocol layer: socket management + HTTP parsing + sending

// Global stop flag set by signal handler to request graceful shutdown
static volatile sig_atomic_t g_stop_requested = 0;

// SIGINT handler: set the stop flag so the main loop can exit cleanly
static void sigint_handler(int signum) {
	std::cout << "\n" << GOLD << "✨ Signal " << signum << " received, cleaning up gracefully! ✨" << RESET << std::endl;
	g_stop_requested = 1;
}

// TODO: Person 2/3 implements this
// Takes parsed request, returns response to send
std::string handleRequest(const Request &req) {
	// PLACEHOLDER: Person 2 will implement routing/file serving
	(void)req; // Suppress unused warning
	return "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
}

// Send HTTP response to client
void sendResponse(int client_fd, const std::string &response) {
	send(client_fd, response.c_str(), response.size(), 0);
}

int run() {
	// 1. Create TCP listening socket
	std::cout << GOLD << "🚧 Setting up servers..." << D << std::endl;
	std::cout << BBLU << "🔧 Creating server sockets..." << D << std::endl;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		throw SocketException("socket failed");

	// Allow quick reuse of the address after program restart
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(server_fd);
		throw SocketException("setsockopt failed");
	}

	// 2. Bind to port 8080
	sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);

	//NEED TO USE THE PORT FROM THE CONFIG.conf
	//address.sin_port = htons(config.getPort());

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		close(server_fd);
		throw SocketException("bind failed");
	}

	// 3. Start listening
	if (listen(server_fd, 16) < 0) {
		close(server_fd);
		throw SocketException("listen failed");
	}

	std::cout << GRN << "✅ Server running at 🌐 " << BGRN << "http://127.0.0.1:8080" << D << std::endl;

	// Install signal handlers: SIGINT for graceful shutdown, ignore SIGPIPE
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	// Main server loop
	for (;;) {
		if (g_stop_requested)
			break;

		// 4. Accept new connection
		socklen_t addrlen = sizeof(address);
		int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
		if (client_fd < 0) {
			if (errno == EINTR) {
				if (g_stop_requested)
					break;
				continue;
			}
			perror("accept failed");
			continue;
		}

		// 5. Read raw request from socket
		const size_t MAX_REQUEST_SIZE = 64 * 1024;
		std::string req_data;
		char readbuf[4096];
		ssize_t n = 0;
		bool headers_complete = false;
		size_t header_end = std::string::npos;

		// Read until we find end of headers (\r\n\r\n)
		while ((n = read(client_fd, readbuf, sizeof(readbuf))) > 0) {
			req_data.append(readbuf, readbuf + n);
			if (req_data.size() > MAX_REQUEST_SIZE)
				break;
			header_end = req_data.find("\r\n\r\n");
			if (header_end != std::string::npos) {
				headers_complete = true;
				break;
			}
		}

		if (!headers_complete) {
			if (n < 0) perror("read failed");
			if (req_data.empty()) {
				close(client_fd);
				continue;
			}
		}

		// Read body if Content-Length present
		if (header_end != std::string::npos) {
			size_t body_have = req_data.size() - (header_end + 4);
			std::string headers_block = req_data.substr(0, header_end + 4);
			std::string headers_lower = toLower(headers_block);
			size_t cl_pos = headers_lower.find("content-length:");
			if (cl_pos != std::string::npos) {
				size_t val_start = cl_pos + strlen("content-length:");
				size_t val_end = headers_lower.find('\r', val_start);
				if (val_end == std::string::npos)
					val_end = headers_lower.find('\n', val_start);
				if (val_end == std::string::npos)
					val_end = headers_lower.size();
				std::string cl_val = trim(req_data.substr(val_start, val_end - val_start));
				int content_len = stringToInt(cl_val);
				if (content_len > 0) {
					while ((int)body_have < content_len && req_data.size() < MAX_REQUEST_SIZE) {
						n = read(client_fd, readbuf, sizeof(readbuf));
						if (n <= 0) break;
						req_data.append(readbuf, readbuf + n);
						body_have = req_data.size() - (header_end + 4);
					}
				}
			}
		}

		// Detect TLS and reject
		if (looks_like_tls(req_data.c_str(), (int)req_data.size())) {
			std::cout << "TLS detected - use http:// not https://" << std::endl;
			close(client_fd);
			continue;
		}

		// 6. Parse request (YOUR HANDOFF TO PERSON 2/3)
		try {
			Request req(req_data);

			if (!req.isValid()) {
				std::cerr << "Invalid request received" << std::endl;
				close(client_fd);
				continue;
			}


			//DEBUUUGGGGGG
			// Print headers (keys are normalized to lowercase)
			const std::map<std::string, std::string>& headers = req.getHeaders();
			for (std::map<std::string, std::string>::const_iterator it2 = headers.begin(); it2 != headers.end(); ++it2) {
				std::cout << it2->first << ": " << it2->second << std::endl;
			}
			// Log basic request information for debugging
			std::cout << "Received request:" << std::endl;
			std::cout << req.getMethod() << " " << req.getPath() << " " << req.getVersion() << std::endl;
			// Print the body (may be empty)
			std::cout << "Body: " << req.getBody() << std::endl;
			//END DEBUG
			
			std::cout << "CLIENT_FD: " << client_fd << std::endl;
			
			// 7. Get response from Person 2/3 (routing + file serving)
			std::string response = handleRequest(req);

			// 8. Send response (MINE RESPONSIBILITY)
			sendResponse(client_fd, response);
		} catch (const RequestException &e) {
			std::cerr << "Parse error: " << e.what() << std::endl;
			// Could send 400 Bad Request here
			close(client_fd);
			continue;
		}
	}

	// Graceful shutdown
	std::cout << BCYAN << "👋 BYE BYE 🔒 Server shut down..." << D << std::endl;
	close(server_fd);
	return 0;
}

int main() {
	try {
		return run();
	} catch (const std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
}