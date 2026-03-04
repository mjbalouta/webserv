#include "Request.hpp"
#include "Exception.hpp"
#include "Utils.hpp"
#include "Includes.hpp"

// Simple single-threaded HTTP demo server.
// Meant for learning and testing: limited parser, no concurrency, no TLS.

// Global stop flag set by signal handler to request graceful shutdown
static volatile sig_atomic_t g_stop_requested = 0;

// SIGINT handler: set the stop flag so the main loop can exit cleanly
static void sigint_handler(int /*signum*/) {
	g_stop_requested = 1;
}

// Function to send a basic HTTP response
void sendResponse(int client_fd, const std::string &body) {
	std::ostringstream response;
	response << "HTTP/1.1 200 OK\r\n";
	response << "Content-Length: " << body.size() << "\r\n";
	response << "Content-Type: text/html\r\n";
	response << "\r\n";
	response << body;

	std::string respStr = response.str();
	// NOTE: send may write fewer bytes than requested; production code
	// should loop until all bytes are sent and check for errors.
	send(client_fd, respStr.c_str(), respStr.size(), 0);
}

int run() {
	// Create a TCP listening socket (IPv4)
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		throw SocketException("socket failed");

	// Allow quick reuse of the address after program restart
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(server_fd);
		throw SocketException("setsockopt failed");
	}

	sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);

	// Bind the socket to the chosen port on all interfaces
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		close(server_fd);
		throw SocketException("bind failed");
	}

	// Start listening with a modest backlog
	if (listen(server_fd, 16) < 0) {
		close(server_fd);
		throw SocketException("listen failed");
	}

	std::cout << "Server listening on port 8080..." << std::endl;

	// Install signal handlers: SIGINT for graceful shutdown, ignore SIGPIPE
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // do not set SA_RESTART so accept can be interrupted
	sigaction(SIGINT, &sa, NULL);
	// Ignore SIGPIPE to avoid termination when writing to closed sockets
	signal(SIGPIPE, SIG_IGN);

	for (;;) {
		if (g_stop_requested) break; // graceful shutdown requested
		socklen_t addrlen = sizeof(address);
		// Accept a new client connection (blocking). Returns a new fd.
		int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
		if (client_fd < 0) {
			if (errno == EINTR) {
				// Interrupted by signal; if stop requested exit loop
				if (g_stop_requested)
					break;
				continue; // otherwise continue accepting
			}
			perror("accept failed");
			continue; // on other error try accepting again
		}

		// Read the request robustly: keep reading until we have reached the end
		// of headers ("\r\n\r\n") and then read the body according to
		// Content-Length, if any. This prevents truncation for larger requests.
		const size_t MAX_REQUEST_SIZE = 64 * 1024; // 64 KiB max to avoid abuse
		std::string req_data;
		char readbuf[4096];
		ssize_t n = 0;
		bool headers_complete = false;
		size_t header_end = std::string::npos;

		// Read loop: gather bytes until headers end ("\r\n\r\n") or socket closes.
		// We append each read into `req_data` and check for the header terminator.
		while ((n = read(client_fd, readbuf, sizeof(readbuf))) > 0) {
			// Append exactly n bytes from the buffer
			req_data.append(readbuf, readbuf + n);
			// Guard: stop reading if request becomes too large
			if (req_data.size() > MAX_REQUEST_SIZE)
				break;
			// Look for the double-CRLF that separates headers and body
			header_end = req_data.find("\r\n\r\n");
			if (header_end != std::string::npos) {
				headers_complete = true;
				break; // headers are complete, break to possibly read body
			}
		}

		if (!headers_complete) {
			// Either socket closed or headers too large — try to parse what we have
			if (n < 0) {
				perror("read failed");
			}
			// If no data at all, close client and continue
			if (req_data.empty()) {
				close(client_fd);
				continue;
			}
		}

		// If headers were found, check for Content-Length and read remaining body
		if (header_end != std::string::npos) {
			// How many body bytes we already have (bytes after the header terminator)
			size_t body_have = req_data.size() - (header_end + 4);
			// Extract header block to inspect headers in a case-insensitive way
			std::string headers_block = req_data.substr(0, header_end + 4);
			std::string headers_lower = headers_block;
			for (size_t i = 0; i < headers_lower.size(); ++i) headers_lower[i] = static_cast<char>(std::tolower((unsigned char)headers_lower[i]));
			size_t cl_pos = headers_lower.find("content-length:");
			if (cl_pos != std::string::npos) {
				// Find start of numeric value after "content-length:"
				size_t val_start = cl_pos + strlen("content-length:");
				// find end of the header line (CR or LF)
				size_t val_end = headers_lower.find('\r', val_start);
				if (val_end == std::string::npos)
					val_end = headers_lower.find('\n', val_start);
				if (val_end == std::string::npos)
					val_end = headers_lower.size();
				// Extract and trim the content-length value
				std::string cl_val = trim(req_data.substr(val_start, val_end - val_start));
				int content_len = stringToInt(cl_val);
				if (content_len > 0) {
					// Read until we have the declared body length or reach limits
					while ((int)body_have < content_len && req_data.size() < MAX_REQUEST_SIZE) {
						n = read(client_fd, readbuf, sizeof(readbuf));
						if (n <= 0)
							break; // socket closed or error
						req_data.append(readbuf, readbuf + n);
						body_have = req_data.size() - (header_end + 4);
					}
				}
			}
		}

		// At this point req_data should contain at least the headers, and body if small or content-length satisfied
		if (looks_like_tls(req_data.c_str(), (int)req_data.size())) {
			std::cout << "Received TLS/HTTPS handshake bytes — likely client used https://\n";
			std::cout << "If you intended plaintext HTTP, use http://localhost:8080 instead.\n";
			close(client_fd);
			continue;
		}

		// Parse the accumulated request data using Request class constructor
		try {
			Request req(req_data);

			// Check if parsing was successful
			if (!req.isValid()) {
				std::cout << "Invalid request received" << std::endl;
				close(client_fd);
				continue;
			}

			// Log basic request information for debugging
			std::cout << "Received request:" << std::endl;
			std::cout << req.getMethod() << " " << req.getPath() << " " << req.getVersion() << std::endl;

			// Print headers (keys are normalized to lowercase)
			const std::map<std::string, std::string>& headers = req.getHeaders();
			for (std::map<std::string, std::string>::const_iterator it2 = headers.begin(); it2 != headers.end(); ++it2) {
				std::cout << it2->first << ": " << it2->second << std::endl;
			}
			// Print the body (may be empty)
			std::cout << "Body: " << req.getBody() << std::endl;

			sendResponse(client_fd, "Hello World! Daniel says eae bitch!");
		} catch (const RequestException &e) {
			std::cerr << "Request parsing error: " << e.what() << std::endl;
			// Could send 400 Bad Request here
			close(client_fd);
			continue;
		}

		close(client_fd);
	}

	// Begin shutdown sequence: close listening socket and exit
	std::cout << "Shutting down server..." << std::endl;
	close(server_fd);
	return 0;
}

int main()
{
	// Entrypoint: run the server. `run()` owns the listening socket lifecycle
	// and will block serving clients until the process is terminated.
	try {
		//Config config("config.conf");
		run();
	} catch (const std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		1;
	}
	return 0;
}