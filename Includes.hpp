#pragma once

// Standard C++ Library
#include <string>      // std::string
#include <vector>      // std::vector
#include <map>         // std::map
#include <exception>   // std::exception
#include <stdexcept>   // std::runtime_error
#include <iostream>    // std::cout, std::cerr
#include <sstream>     // std::ostringstream, std::istringstream
#include <fstream>      // std::ifstream, std::ostream

// C Standard Library
#include <cstring>     // memset, strlen
#include <cstdio>      // perror
#include <cstdlib>     // atoi, strtol
#include <cctype>      // isspace, tolower, toupper, isalnum
#include <ctime>       // time
#include <unistd.h>
#include <dirent.h>    // DIR *

// POSIX/System Headers
#include <sys/socket.h> // socket, bind, listen, accept, send
#include <sys/stat.h>   // stat
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons
#include <unistd.h>     // read, write, close
#include <signal.h>     // signal, sigaction, SIGINT, SIGPIPE
#include <errno.h>      // errno, EINTR

#include <sys/epoll.h>
#include <arpa/inet.h> //inet_addr
#include <fcntl.h> //fnctl

#define MAX_EVENTS 64
#define BUFFER_SIZE 4096
#define KEEP_ALIVE_TIMEOUT 15

// Custom Class Implementations and .hpp headers
#include "PathResolver.hpp"
class PathResolver;
#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "Utils.hpp"
#include "Exception.hpp"

// ANSI Color Codes for Terminal Output
#define RESET   "\033[0m"       // Reset all attributes
#define B       "\033[1m"       // Bold
#define RED     "\033[31m"      // Red text
#define BRED    "\033[1;31m"    // Bold red
#define GRN     "\033[32m"      // Green text
#define BGRN    "\033[1;32m"    // Bold green
#define YEL     "\033[33m"      // Yellow text
#define BYEL    "\033[1;33m"    // Bold yellow
#define BLU     "\033[34m"      // Blue text
#define BBLU    "\033[1;34m"    // Bold blue
#define MAG     "\033[35m"      // Magenta text
#define BMAG    "\033[1;35m"    // Bold magenta
#define CYAN    "\033[36m"      // Cyan text
#define BCYAN   "\033[1;36m"    // Bold cyan
#define WHT     "\033[37m"      // White text
#define BWHT    "\033[1;37m"    // Bold white
#define GOLD    "\033[38;5;220m" // Gold/yellow text (extended color)
#define BGOLD   "\033[1;38;5;220m" // Bold gold
#define SILV    "\033[38;5;250m" // Silver/gray text (extended color)
#define BSILV   "\033[1;38;5;250m" // Bold silver
#define D       "\033[0m"       // Dark/Reset (alias for RESET)
#define BOLD    "\033[1m"
