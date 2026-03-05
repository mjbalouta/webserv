#pragma once

// Standard C++ Library
#include <string>      // std::string
#include <map>         // std::map
#include <exception>   // std::exception
#include <iostream>    // std::cout, std::cerr
#include <sstream>     // std::ostringstream, std::istringstream
#include <vector>       // std::vector

// C Standard Library
#include <cstring>     // memset, strlen
#include <cstdio>      // perror
#include <cstdlib>     // atoi, strtol
#include <cctype>      // isspace, tolower, toupper, isalnum
#include <ctime>       // time

// POSIX/System Headers
#include <sys/socket.h> // socket, bind, listen, accept, send
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons
#include <unistd.h>     // read, write, close
#include <signal.h>     // signal, sigaction, SIGINT, SIGPIPE
#include <errno.h>      // errno, EINTR


// Custom Class Implementations and .hpp headers
// #include "PathResolver.hpp"
class PathResolver;


// ANSI Color Codes for Terminal Output
#define RESET   "\033[0m"       // Reset all attributes
#define B       "\033[1m"       // Bold
#define RED     "\033[31m"      // Red text
#define GRN     "\033[32m"      // Green text
#define BGRN    "\033[1;32m"    // Bold green
#define BLU     "\033[34m"      // Blue text
#define BBLU     "\033[1;34m"      // Blue text
#define CYAN    "\033[36m"      // Cyan text
#define BCYAN   "\033[1;36m"    // Bold cyan
#define GOLD    "\033[38;5;220m" // Gold/yellow text (extended color)
#define BGOLD   "\033[1;38;5;220m" // Bold gold
#define SILV    "\033[38;5;250m" // Silver/gray text (extended color)
#define BSILV   "\033[1;38;5;250m" // Bold silver
#define D       "\033[0m"       // Dark/Reset (alias for RESET)
#define BOLD    "\033[1m"
