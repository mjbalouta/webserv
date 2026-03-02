# webserv

https://github.com/users/mjbalouta/projects/4

## Person 1 — HTTP & Connection Engine
### Responsibility

Handles everything related to:
- TCP connections
- Client lifecycle
- HTTP request parsing
- Sending responses

This layer is the transport & protocol layer of the server.

### Tasks
1️⃣ Socket Management

- Create server socket (socket)
- Bind to IP and port (bind)
- Start listening (listen)
- Accept new connections (accept)
- Handle multiple clients (select / poll if required)
- Close connections safely

2️⃣ HTTP Request Parsing

- Convert raw HTTP text into a structured Request object.

Example raw request:

`GET /index.html HTTP/1.1
Host: localhost:8080
Content-Type: text/html`

Example structure:

`class Request {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};`

Must handle:
- Request line parsing
- Header extraction
- Detecting \r\n\r\n
- POST body handling
- Content-Length
- Partial reads
- Malformed requests

3️⃣ Sending Responses

- Convert Response object to raw HTTP string
- Ensure correct formatting
- Ensure correct Content-Length
- Send via socket

### Interface
Outputs:
`Request request;`
Receives:
`Response response;`

### Learning Focus
- TCP networking
- HTTP protocol internals
- Connection lifecycle
- Real-world server behavior

## Person 2 — Configuration & Routing Engine
### Responsibility

Handles:
- Config file parsing
- Server and location rules
- Method validation
- Routing decisions
- URL resolution

This layer is the decision engine of the server.

### Tasks
1️⃣ Configuration Parsing

Parse configuration file such as:

`server {
    listen 8080;
    root ./www;
    index index.html;
    error_page 404 ./errors/404.html;
    location /images {
        root ./assets;
        allow_methods GET;
    }
}`

Must implement:
- Tokenizer
- Nested {} block parsing
- Directive validation
- Syntax error detection
- Multiple server support

Internal Structures
`class LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> allowedMethods;
};`

`class ServerConfig {
    int port;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    std::vector<LocationConfig> locations;
};`
2️⃣ Routing Logic

Given a Request, determine:
- Which ServerConfig to use
- Which LocationConfig matches best
- If the method is allowed
- Effective root directory
- Final file path
- Status code (if error)

Example result object:
`class RouteResult {
    std::string fullPath;
    bool isDirectory;
    bool methodAllowed;
    int statusCode;
};`

### Interface

Inputs:
`Request request;
std::vector<ServerConfig> configs;`

Outputs:
`RouteResult result;`

### Learning Focus
- Configuration systems
- URL routing mechanics
- Separation of concerns
- Clean architecture design

## Person 3 — Resource & Response Engine
### Responsibility

Handles:
- File system access
- Directory handling
- MIME type detection
- Error page generation
- Building final Response
- Full server orchestration

This layer is the content & stability engine.

### Tasks
1️⃣ File Handling

Given a RouteResult, must:
- Normalize paths
- Prevent directory traversal
- Check file existence
- Detect directories
- Serve index files
- Read file content
- Handle large files
- Determine MIME type

2️⃣ Error Handling

Generate responses for:
- 404 Not Found
- 403 Forbidden
- 405 Method Not Allowed
- 500 Internal Server Error

Must:
- Serve custom error pages if defined
- Always return valid HTTP response
- Never crash server

3️⃣ Response Construction
`class Response {
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
};`

Must set:
- Status line
- Content-Type
- Content-Length
- Connection header

4️⃣ Server Orchestration

Main loop structure:

`while (true) {
    accept connection
    parse request (P1)
    route request (P2)
    build response (P3)
    send response (P1)
}`

Responsible for full system integration.
