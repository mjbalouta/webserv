#pragma once

#include "Includes.hpp"

void printLog(std::string message, std::string color);
std::string itostr(int value);
std::string& trimSpaces(std::string& str);
long strToLong(const std::string& value);
void addToEpoll(int epollFd, int fd, uint32_t events);
void removeFromEpoll(int epollFd, int fd);
void modEpoll(int epollFd, int fd, uint32_t events);
void setNonBlockingFd(int fd);
void setBlockingFd(int fd);
std::string normalization(std::string &name);
std::string toLower(const std::string &value);
int allDigits(std::string& str);
