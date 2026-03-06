#pragma once
#include "Includes.hpp"

void printMessage(std::string message, std::string color);
std::string itostr(int value);
std::string& trimSpaces(std::string& str);
void addToEpoll(int epollFd, int fd, uint32_t events);
void removeFromEpoll(int epollFd, int fd);
void modEpoll(int epollFd, int fd, uint32_t events);

