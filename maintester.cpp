#include "PathResolver.hpp"
#include <iostream>

int main() {
	PathResolver resolver;
	std::string path = "/a/b/../c/./d//e/";
	std::string normalizedPath = resolver.normalizePath(path);
	std::cout << "Normalized Path: " << normalizedPath << std::endl; // Output should be: /a/c/d/e
	path = "/static/./images/../css//style.css";
	normalizedPath = resolver.normalizePath(path);
	std::cout << "Normalized Path: " << normalizedPath << std::endl; // Output should be: /static/css/style.css
	path = "/a/b/c/../../d";
	normalizedPath = resolver.normalizePath(path);
	std::cout << "Normalized Path: " << normalizedPath << std::endl; // Output should be: /a/d
	return 0;
}