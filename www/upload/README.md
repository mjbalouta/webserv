*This project has been created as part of the 42 curriculum by dda-fons, mipinhei, mjoao-fr.*

# Description

This project is about developing our own HTTP server in C++98. HTTP is one of the most widely used protocols on the internet.
The primary function of a web server is to store, process, and deliver web pages to clients. Client-server communication occurs through the Hypertext Transfer Protocol (HTTP).
This project is designed to handle non-blocking I/O using a multiplexing system (epoll), allowing it to serve multiple clients simultaneously without performance degradation.
The server's core responsibility is to parse incoming HTTP requests, match them against a flexible configuration file, and deliver the appropriate resources—whether they are static files, CGI script outputs, or custom error pages.

# Instructions

## Prerequisites
This project is designed for Unix-based systems (Linux/MacOS). Ensure you have a C++ compiler (like g++ or clang++) and make installed.

## Compilation
To compile the project, run: `make`.

## Execution
To launch the server, you must provide a configuration file as an argument. This file must follow specific nginx syntax rules.   
If no file is provided, a default configuration can be used with `make run`.   
If you have a config file: `./webserv [path_to_config_file]`.

## Testing
Once the server is running, you can interact with it using a web browser or `curl`.   
   
Some tests include:   
A. Check if the server is alive (GET): `curl -v http://localhost:8080/`   
B. Test a File Upload (POST): `curl -v -X POST -d "Hello Webserver" http://localhost:8080/upload/test.txt`   
C. Test CGI Execution: `curl -v http://localhost:8080/cgi-bin/hello.py`   

# Resources
https://nginx.org/   
AI tools were used during this project's development to assist with task organization, conceptual research, and architectural workflow design. It served as an educational aid to deepen our understanding of a webserver flow.

## From our Notebooks

### What is Nginx?   
It's a high-performance, open-source HTTP server and reverse proxy. It is used as a reference in this project because, while old servers often create a new "thread" or "process" for every single visitor (which consumes a lot of memory), Nginx revolutionized the field with an Asynchronous, Event-Driven Architecture. Our project also uses Non-Blocking I/O (single process to handle thousands of connections simultaneously); Modular Configuration (a hierarchical "block" system - server and location blocks); Efficiency (high concurrency with very low memory usage, achieved through the same multiplexing logic - epoll - used in this project).

### WEBSERVER FLOW
"building a webserver is like building a high-speed post office that never sleeps."   
   
**1. THE SETUP:**   
- Parsing: reads from a config file to know which port to listen on.
- Socket Creation: creates a socket, which is an endpoint for communication.
- Binding and Listening: it tells the operating system "I am now the owner of port xxxx, if anyone knocks, tell me".   
   
**Socket:** the internal endpoint for sending or receiving data across a network.   
   
**The lifecycle of a socket:**   
a) Creation: you tell the OS "I need a socket for IPv4 and TCP".      
b) Binding: you attach that socket to a specific port and an IP address.   
c) Listening: you put the socket into a "passive" state, waiting for someone to knock.   
d) Accepting: when a client (like chrome or curl) connects, accept creates a new socket specifically for that one client. The original "listening" socket stays open to wait for the next person.   
e) Communication: you read the HTTP request from the client socket and send back to HTTP response.   

**Types of sockets:**   
a) The Listening Socket: this is the "master" socket. Its only job is to wait for new connections. Usually there is one of these for each server block in the config file (that has a unique port).   
b) The Client Socket: everytime a user connects, accept() gives a new file descriptor. This represents the direct line to that specific client. If 50 people are visiting the website, there will be 50 sockets open.   
   
**2. THE WAITING ROOM:**   
This is where your epoll() lives. The server sits in a loop, asking the OS: "Is there any new data? Did a new person connect? Has a previous client finished sending their request?".
This is where the traffic is controlled: since the server needs to handle multiple clients at the same time without using threads, we can't just call recv() and wait. If we did, and that one client didn't send anything, the whole server would freeze and no one else would be able to connect. Epoll (I/O Multiplexing Functions) allow the server to ask the OS "of these 100 sockets, which ones have someone standing there with data right now?".   
   
**How the waiting room works:**   
a) The Watchlist: you give epoll() a list of all your file descriptors (the listening sockets + all currently connected client sockets);   
b) The Sleep: you call epoll();   
c) The Wake-up: as soon as data arrives on any of these sockets, the OS wakes your program up and tells you exactly which sockets are "ready".   
d) The Action: you loop through the results. If the listening socket is ready, someone is trying to connect (call accept()); if a client socket is ready, someone sent a request (call recv()).   
   
**3. THE PROCESSING:**   
When a request arrives, it must go through these steps:   
- The Parser: turns the RAW HTTP request text into a Request object;   
- The Router: takes that Request, looks at the Config, and decides what config file info is relevant for the Response generation;   
- The Content Builder: reads the file from the disk or generates an error page.   
   
**How is a request generated?**   
An HTTP request is generated by a client (like a web browser, curl,...) and sent over a TCP socket to your server. It is just a plain text string formatted in a very specific way.   
***The Trigger:***   
i. DNS Lookup: the browser finds the IP address for the hostname;   
ii. TCP Handshake: the browser opens a connection to your server's IP and Port;   
iii. The Generation: once the connection is open, the browser constructs a text block.   
   
**The Structure of the Request:**   
The request is divided into three main parts. This entire block is received as one big string via recv().   
   
*A. The Request Line:* the first line and it tells the server what the client wants:   
- Method: GET, POST OR DELETE;
- Path: /index.html;
- Version: HTTP/1.1

*B. Header:* these provide metadata about the request. Each line ends with a carriage return and line feed - `\r\n`.   
- Host: localhost:8080;
- User-Agent: Mozilla/5.0;
- Content-Length: 42;   
   
*C. The Empty Line:* there is always exactly one empty line between the headers and the body. This is how the server knows the header have ended.   
   
*D. The Body:* (optional) Used in POST request to send data.   
   
Important: Sometimes a large request won't arrive all at once. The server must be able to "buffer" the data until the double "\r\n\r\n" that signals the end of the headers.   
   
**4. THE HANDSHAKE:**   
The server sends the bytes to the client and decides whether to close the connection or keep-alive to wait for another request.

**The Response Process:**   
1. The Decision Tree:   
	i. case A: redirection (if getReturnStatusCode() is 301 or 302, it stops everything it was supposed to do and just write header: Location:[getReturnURL()].);   
	ii. case B: error (if you set an error status, the response person will look through getErrorPages() and check if there is a custom .html file for that error. If yes, they read it, if no, they generate a "Default Error".);   
	iii. case C: success (if the status is 200, they move to the next step.).   
2. The Physical Search:   
The Response person takes the getResolvedPath() and tries to open that file:   
	i. If it's a file: they read the bytes into a buffer;   
	ii. If it's a directory: they check getAutoIndex() (If ON: they use opendir() and readdir() to list every file in the folder and wrap them in tags to make a clickable HTML page; if OFF: they look through getIndexes() to see if any of those exist in that folder).   
3. The Translation to HTTP:   
Now the Response person has to turn a file into a string of text the browser understands. They build the Response Header by asking more questions:   
	i. MIME Type: they look at the file extension (.png) and set Content-Type: image/png;   
	ii. Size: they check the file size and set Content-Length: 1024;   
	iii. Allowed Methods: if the request was a GET, but the getAllowedMethods() only has a POST, it sends back a 405 instead of the file.   
4. The Handoff to the Network:   
The Response person doesn't send the data themselves. They fill up a string or a buffer inside the ClientSession and change the client.state to WRITING. The ServerManager sees that the state is now WRITING and starts calling send() on the socket until all those bytes are gone.   
   
      
**The Client States:**   
1. READING:   
. the server is calling recv()/read(). Since the internet is slower than your CPU, the request usually arrives in chunks;   
. it keeps reading until it catches the end of headers marker;   
. once the full request (including the body) is in the buffer, the state flips to PROCESSING. If they send a file that is greater than the limit, the client state is switched again to WRITING and immediately sends 413 Payload Too Large;   
2. PROCESSING:   
. the request is split into Method, Path and Headers;   
. the routing matches the path to a location block, or a server block if there are no location blocks;   
. the response builder finds the file on disk or prepares de CGI script;   
. once the HTTP response string is fully built and ready, the state flips to WRITING.   
3. WRITING:   
. the server calls send(). If you're sending a large image, the network "pipe" might get full - the server send a chunk, sees if it can send more and stays in the WRITING state;   
. the epoll: "wake me up when this client's mailbox is empty so I can send the next chunk";   
. once the last byte is sent: if connection is keep-alive, it flips back into READING state waiting for a new request; if the connection is close, it flips to CLOSING.   
4. CLOSING:   
. close(fd) is called;   
. this is the most important part for stability. It releases the File Descriptor. If you don't reach this state, your server will eventually run out of 'slots' and stop accepting new people.






