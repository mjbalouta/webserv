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
	path = "/a/b/////c/../../d";
	normalizedPath = resolver.normalizePath(path);
	std::cout << "Normalized Path: " << normalizedPath << std::endl; // Output should be: /a/d

	std::string root = "/var/www/html";
	std::string requested = "/static/../index.html";
	bool safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe: " << (safe ? "Yes" : "No") << std::endl; // Output should be: Yes

	requested = "/static/../../etc/passwd";
	safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe: " << (safe ? "Yes" : "No") << std::endl; // Output should be: No

	requested = "/static/css/style.css";
	safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe: " << (safe ? "Yes" : "No") << std::endl; // Output should be: Yes

	requested = "/static/../../../";
	safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe: " << (safe ? "Yes" : "No") << std::endl; // Output should be: No

	root = "";
	requested = "/any/path/should/be/safe";
	safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe with empty root: " << (safe ? "Yes" : "No") << std::endl; // Output should be: Yes

	root = "/var/../html";
	requested = "/index.html";
	safe = resolver.isPathSafe(requested, root);
	std::cout << "Is Path Safe with normalized root: " << (safe ? "Yes" : "No") << std::endl; // Output should be: Yes
	return 0;
}