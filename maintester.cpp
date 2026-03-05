#include "PathResolver.hpp"
#include "FileSystemHandler.hpp"
#include <iostream>

int main() {
//	PathResolver resolver;
/* 	std::string path = "/a/b/../c/./d//e/";
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
	std::string path = resolver.resolveToFilesystem("/static/../index.html", "/var/www/html");
	std::cout << "Resolved Filesystem Path: " << path << " Is Path Safe: " << (resolver.isPathSafe(path, "/var/www/html") ? "Yes" : "No") << std::endl; // Output should be: /var/www/html/index.html
	path = resolver.resolveToFilesystem("/static/../../etc/passwd", "/var/www/html");
	std::cout << "Resolved Filesystem Path: " << path << " Is Path Safe: " << (resolver.isPathSafe(path, "/var/www/html") ? "Yes" : "No") << std::endl; // Output should be: /var/www/html/../../etc/pass
	path = resolver.resolveToFilesystem("/static/css/style.css", "/var/www/html");
	std::cout << "Resolved Filesystem Path: " << path << " Is Path Safe: " << (resolver.isPathSafe(path, "/var/www/html") ? "Yes" : "No") << std::endl; // Output should be: /var/www/html/static/css/style.css
	path = resolver.resolveToFilesystem("/static/../../../", "/var/www/html");
	std::cout << "Resolved Filesystem Path: " << path << " Is Path Safe: " << (resolver.isPathSafe(path, "/var/www/html") ? "Yes" : "No") << std::endl; // Output should be: /var/www/html/static/../../../ */
	FileSystemHandler fsHandler;
	std::string filepath = ("text.txt");
	std::cout << "Does path exist? " << (fsHandler.pathExists(filepath) ? "Yes" : "No") << std::endl;
	std::cout << "Is it a directory? " << (fsHandler.isDirectory(filepath) ? "Yes" : "No") << std::endl;
	std::cout << "Is it readable? " << (fsHandler.isReadable(filepath) ? "Yes" : "No") << std::endl;
	std::string content = fsHandler.readFile(filepath);
	std::cout << "File content: " << content << std::endl;
	
	return 0;
}