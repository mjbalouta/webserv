#include "Utils.hpp"

/**
 * @brief Prints a colored message to standard output
 * @param message The message string to print
 * @param color ANSI color code to apply to the message
 * @return void
 */
void printMessage(std::string message, std::string color)
{
	std::cout << color << message << D << std::endl;
}

/**
 * @brief Converts an integer to a string (C++98 compatible)
 * @param value The integer value to convert
 * @return std::string The string representation of the integer
 */
std::string itostr(int value){
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

/**
 * @brief Searches for whitespaces in the beginning or end of a str
 * and trims them
 * 
 * @param str 
 * @return std::string& 
 */
std::string& trimSpaces(std::string& str)
{
	size_t start = str.find_first_not_of(" \t\n\r");
	if (start == std::string::npos)
	{
		str.clear();
		return str;
	}

	size_t end = str.find_last_not_of(" \t\n\r");
	str.erase(0, start);
	str.erase(end + 1);

	return str;
}

/**
 * @brief Adds a file descriptor to the epoll instance for monitoring
 * @param fd File descriptor to add
 * @param events Epoll events to monitor (e.g., EPOLLIN for read readiness)
 * @throw std::runtime_error if epoll_ctl fails
 */
void addToEpoll(int epollFd, int fd, uint32_t events) {
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = fd;
	
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) { //Calls epoll_ctl() to register this fd. _epollFd - epoll instance. EPOLL_CTL_ADD - operation: add a new fd. fd - the socket to monitor. &ev - pointer to the event config
		throw std::runtime_error("Failed to add fd " + itostr(fd) + " to epoll");
	}
}

/**
 * @brief Removes a file descriptor from the epoll instance for monitoring
 * @param fd File descriptor to remove
 * @throw std::runtime_error if epoll_ctl fails (fd was not registered)
 */
void removeFromEpoll(int epollFd, int fd) {
	if (fd < 0)
		return;  // Skip invalid fds
	if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) { //Calls epoll_ctl() to unregister this fd. _epollFd - epoll instance. EPOLL_CTL_DEL - operation: remove fd. fd - the socket to stop monitoring. NULL - not used for DEL operation
		throw std::runtime_error("Failed to remove fd " + itostr(fd) + " from epoll");
	}
}

/**
 * @brief Modifies events monitored for an existing file descriptor in epoll
 * @param fd File descriptor to update
 * @param events New epoll events to monitor (e.g., EPOLLIN, EPOLLOUT)
 * @throw std::runtime_error if epoll_ctl fails (fd was not registered)
 */
void modEpoll(int epollFd, int fd, uint32_t events)
{
	if (fd < 0)
		return;  // Skip invalid fds

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) { //Calls epoll_ctl() to modify this fd. epollFd - epoll instance. EPOLL_CTL_MOD - operation: update monitored events. fd - the socket to update. &ev - new event configuration
		throw std::runtime_error("Failed to modify fd " + itostr(fd) + " in epoll");
	}
}