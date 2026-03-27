#include "../config/ServerConfig.hpp"
#include "../Includes.hpp"
#include "../Utils.hpp"
#include "../ServerManager/Request.hpp"

/**
 * @brief Holds the pipe fds and process info for one active CGI child.
 *
 * Created by CgiHandler::start() and stored inside ClientSession.
 * All fds are non-blocking. ServerManager registers them in epoll.
 */
struct CgiProcess {
	pid_t  pid;       // Child PID returned by fork()
	int    writeFd;   // pipe_in[1]  — parent writes POST body here
	int    readFd;    // pipe_out[0] — parent reads CGI output here
	time_t startTime; // epoch time of fork(), used for timeout detection

	CgiProcess() : pid(-1), writeFd(-1), readFd(-1), startTime(0) {}
};


class CgiHandler {
public:

	static CgiProcess start(const Request &request,
							 const std::string &scriptPath,
							 const std::string &interpreter,
							 const ServerConfig &server);

	static std::string buildResponse(const std::string &rawOutput, const std::string &httpVersion, bool keepAlive);

private:
	static void buildEnv(const Request &request,
							const std::string &scriptPath,
							const ServerConfig &server,
							std::vector<std::string> &envStrings,
							std::vector<char *> &envp);

};