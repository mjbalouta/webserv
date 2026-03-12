#include "Utils.hpp"

/**
 * @brief Checks if the string only contains a-z or A-Z
 * 
 * @param str 
 * @return int 
 */
int allLetters(std::string& str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] < 'a' || str[i] > 'z')
			return 0;
		if (str[i] < 'A' || str[i] > 'Z')
			return 0;
	}
	return 1;
}

/**
 * @brief Checks if the string only contains digits from '0' to '9'
 * 
 * @param str 
 * @return int 
 */
int allDigits(std::string& str)
{
	size_t pos = str.find_first_not_of("0123456789");
	if (pos != std::string::npos)
		return 0;
	return 1;
}

/**
 * @brief Prints a colored message to standard output
 * @param message The message string to print
 * @param color ANSI color code to apply to the message
 * @return void
 */
void printLog(std::string message, std::string color)
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
	str = str.substr(start, end - start + 1);

	return str;
}

/**
 * @brief Converts a string to a long integer.
 * @param value Input string that may include leading/trailing whitespace.
 * @return long Parsed value from base-10 conversion.
 * @throw std::runtime_error If the string is empty after trimming.
 */
long strToLong(const std::string& value)
{
	std::string trimmed = value;
	trimSpaces(trimmed);
	if (trimmed.empty())
		throw std::runtime_error("Invalid number: empty string");
	char *end = NULL;
	long result = ::strtol(trimmed.c_str(), &end, 10);
	return result;
}

void setNonBlockingFd(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		throw std::runtime_error("fcntl F_GETFL failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl F_SETFL failed");
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

/**
 * @brief Creates a lowercase version of the given string.
 * @param value Input text to normalize.
 * @return std::string A new string where each character is converted to lowercase.
 */
std::string toLower(const std::string &value)
{
	std::string result = value;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::tolower(result[i]));
	return result;
}