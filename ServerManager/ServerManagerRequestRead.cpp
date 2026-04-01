#include "ServerManager.hpp"

static const size_t MAX_HEADER_SIZE = 8192;

/**
 * @brief Checks if a given header line contains a specific token.
 */
static bool hasHeaderToken(const std::string &headersLower, const std::string &headerName, const std::string &token)
{
	// Scan one header line at a time so matches are anchored to real line starts.
	for (size_t lineStart = 0; lineStart < headersLower.size(); )
	{
		// Find the end of the current header line.
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			// Last line in the buffer may not have trailing CRLF.
			lineEnd = headersLower.size(); // Accept only exact header-name match at the beginning of this line.
		// This prevents spoofing via request target text or other header values.
		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, headerName.size(), headerName) == 0)
		{
			// Search token only inside the matched header line.
			return headersLower.substr(lineStart, lineEnd - lineStart).find(token) != std::string::npos;
		}

		if (lineEnd == headersLower.size())
			// Reached the last line.
			break;
		// Move to the next line (skip "\r\n").
		lineStart = lineEnd + 2;
	}
	return 0;
}

/**
 * @brief Parses `Content-Length` from normalized headers when present.
 * @param headersLower Lowercased header block.
 * @param outContentLength Output parsed length.
 * @return true on success or when header is absent, false on invalid value.
 */
static bool parseContentLengthValue(const std::string &headersLower, size_t &outContentLength)
{
	const std::string header = "content-length:";
	// `lineStart` walks line by line; `start/end` delimit the numeric slice.
	size_t lineStart = 0;
	size_t start = std::string::npos;
	size_t end = std::string::npos;

	// Find the Content-Length header only when it appears at a line start.
	for (; lineStart < headersLower.size(); )
	{
		// Locate end of the current header line.
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = headersLower.size();

		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, header.length(), header) == 0)
		{
			// Number starts right after "content-length:" and ends at line end.
			size_t valueStart = lineStart + header.length();
			size_t valueEnd = lineEnd;
			if (start == std::string::npos)
			{
				// First Content-Length header encountered: store its value span.
				start = valueStart;
				end = valueEnd;
			}
			else
			{
				// Second Content-Length header found: reject request.
				// Multiple Content-Length lines are ambiguous and can enable
				// request-smuggling desync between intermediaries and origin server.
				return 0;
			}
		}

		if (lineEnd == headersLower.size())
			// Reached the final line in the buffer.
			break;
		// Advance to next line start (skip over "\r\n").
		lineStart = lineEnd + 2;
	}

	if (start == std::string::npos)
		// Header absent is not automatically an error here; caller decides.
		return true;

	// Skip optional whitespace before the number.
	while (start < end && (headersLower[start] == ' ' || headersLower[start] == '\t'))
		++start;

	// Trim optional trailing spaces/tabs after the numeric value.
	while (end > start && (headersLower[end - 1] == ' ' || headersLower[end - 1] == '\t'))
		--end;

	// Empty value after trimming is invalid (e.g. "content-length:   ").
	if (end <= start)
		return 0;

	// Isolate the raw Content-Length token to parse.
	std::string valueStr = headersLower.substr(start, end - start);
	long value;
	try
	{
		// Shared parser validates range, format, and trailing characters.
		value = strToLong(valueStr);
	}
	catch (const std::exception &)
	{
		// Convert parsing exceptions into this function's bool error contract.
		return 0;
	}

	if (value < 0)
		return 0;

	// Ensure the parsed value can fit in size_t before casting.
	if (static_cast<unsigned long>(value) > std::numeric_limits<size_t>::max())
		return 0;

	// Store validated Content-Length.
	outContentLength = static_cast<size_t>(value);
	return true;
}

/**
 * @brief Reads request bytes and transitions session into processing when complete.
 * @param client Client session.
 * @param maxUploadSize Max accepted body size for this endpoint.
 */
void ServerManager::readClientRequest(ClientSession &client, size_t maxUploadSize, ServerConfig &server)
{
	if (client.fd < 0)
	{
		printLog("🚨 No socket available", RED);
		client.state = CLOSING;
		return;
	}

	// Read in a loop until no more data is available (EAGAIN or EWOULDBLOCK).
	// peerClosed is set when recv() returns 0 (FIN received). We do NOT close
	// immediately — we let the chunked/content-length logic below run first so
	// a complete request that arrived in the same TCP segment as the FIN is
	// still processed and answered before the connection is torn down.
	bool peerClosed = false;
	while (true) {
		char buffer[BUFFER_SIZE];
		int readBytes = recv(client.fd, buffer, sizeof(buffer), 0);
		if (readBytes < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// No more data to read now
				break;
			}
			client.ioFailures++;
			if (client.ioFailures < 2) {
				client.state = READING;
				return;
			}
			printLog("🚨 recv() failed twice consecutively — closing connection", RED);
			client.state = CLOSING;
			return;
		}
		if (readBytes == 0) {
			// Peer sent FIN. Break out so the decoder gets one last chance to
			// finish. peerClosed will close the connection further down if the
			// request turns out to be incomplete.
			peerClosed = true;
			break;
		}
		client.ioFailures = 0;
		client.readBuffer.append(buffer, readBytes);
		client.totalReceived += readBytes;
		if (readBytes < (int)sizeof(buffer))
			break;
	}

	// --- Stream POST body to CGI as it arrives ---
	// If a CGI is active and we are still receiving the body, append new data to cgiInputBuffer
	if (client.cgi.pid > 0 && client.cgi.writeFd >= 0) {
		size_t headerEnd = client.readBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			size_t headerSize = headerEnd + 4;
			size_t alreadyBuffered = client.cgiInputBuffer.size();
			size_t totalBodySize = client.readBuffer.size() - headerSize;
			if (totalBodySize > alreadyBuffered) {
				// Append only the new bytes received
				client.cgiInputBuffer.append(client.readBuffer.substr(headerSize + alreadyBuffered, totalBodySize - alreadyBuffered));
			}
		}
	}

	// Look for the HTTP header terminator: "\r\n\r\n". Until this appears, we only have a partial header block.
	size_t headerEnd = client.readBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{		// If peer closed connection but no complete request headers received yet,
		// close the connection immediately (especially after keep-alive: don't loop forever).
		if (peerClosed)
		{
			printLog("⏳ Client closed before sending a complete request (likely after keep-alive response)", BYEL);
			client.state = CLOSING;
			return;
		}
		// DoS means Denial of Service. It’s an attack where someone makes a server unavailable by exhausting resources like
		// Anti-DoS guard #1:
		// If a client keeps sending bytes without finishing headers,
		// readBuffer would grow forever. Cap header growth at MAX_HEADER_SIZE.
		if (client.readBuffer.size() > MAX_HEADER_SIZE)
		{
			printLog("🚨 Request headers too large", RED);
			// 431 = header section is too large (more precise than generic 413).
			client.status = 431;
			client.keepAlive = false;
			// Move to write path so we send the error response immediately.
			client.state = WRITING;
		}
		return;
	}
	// --- 100 Continue logic ---
	// Only send 100 Continue once per request
	if (!client.headersSent) {
		size_t requestLineEnd = client.readBuffer.find("\r\n");
		size_t headerStart = (requestLineEnd != std::string::npos) ? requestLineEnd + 2 : 0;
		std::string headersLower = toLower(client.readBuffer.substr(headerStart, headerEnd - headerStart));
		if (headersLower.find("expect: 100-continue") != std::string::npos) {
			printLog("[EXPECT 100-CONTINUE] Detected for fd=" + itostr(client.fd), BYEL);
			std::string continueMsg = "HTTP/1.1 100 Continue\r\n\r\n";
			ssize_t sent = send(client.fd, continueMsg.c_str(), continueMsg.size(), MSG_NOSIGNAL);
			if (sent == (ssize_t)continueMsg.size()) {
				// Simple 100 Continue log (C++98 compatible)
				printLog("[100 CONTINUE] Sent to client fd=" + itostr(client.fd), BGRN);
				client.headersSent = true;
			}
			// If send fails, we do not retry here; connection will be closed on next error
		}
		else {
			client.headersSent = true; // Mark as sent to avoid re-checking
		}
	}

	// headerEnd points to the first '\r' of the terminator, so +4 includes "\r\n\r\n".
	size_t headerSize = headerEnd + 4;
	// Anti-DoS guard #2:
	// Even with a terminator, reject oversized header blocks.
	if (headerSize > MAX_HEADER_SIZE)
	{
		printLog("🚨 Request headers too large", RED);
		client.status = 431;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}

	// Header parsing/inspection happens only after size limits pass.
	// Build a lowercase view of the *header lines only* (exclude request line)
	// so token scans cannot be spoofed via method/path/version text.
	size_t requestLineEnd = client.readBuffer.find("\r\n");
	if (requestLineEnd == std::string::npos || requestLineEnd > headerEnd)
	{
		printLog("🚨 Malformed request line or headers", RED);
		client.status = 400;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}
	size_t headerStart = requestLineEnd + 2;
	std::string headersLower = toLower(client.readBuffer.substr(headerStart, headerEnd - headerStart));

	// --- 100 Continue logic ---
	if (headersLower.find("expect: 100-continue") != std::string::npos && !client.headersSent) {
		const char *continueMsg = "HTTP/1.1 100 Continue\r\n\r\n";
		ssize_t sent = send(client.fd, continueMsg, strlen(continueMsg), MSG_NOSIGNAL);
		if (sent > 0) {
			printLog("[100 Continue sent]", BGRN);
		}
		client.headersSent = true; // Prevent sending 100 Continue more than once
	}

	bool hasTransferEncoding = false;
	bool isChunkedOnly = false;
	int teResult = parseTransferEncodingHeader(headersLower, hasTransferEncoding, isChunkedOnly);
	if (teResult == 1) // Malformed
	{
		client.status = 400;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}
	if (teResult == 2) // Unsupported
	{
		client.status = 501;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}
	//if has transfer encoding and content-length, refuse, safe way. client made bad request or smt
	if (hasTransferEncoding && headersLower.find("content-length:") != std::string::npos)
	{
		client.status = 400;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}
	//if is only chunked then decode 
	if (hasTransferEncoding && isChunkedOnly)
	{
		// decodeChunked() is fully incremental: it resumes from chunkedCursor
		// and never re-scans already-decoded data, so it is safe to call on
		// every EPOLLIN event until state becomes PROCESSING.
		decodeChunked(client, maxUploadSize);
		// If the peer already sent FIN and decoding still didn't complete,
		// the body was truncated — close rather than wait forever.
		if (peerClosed && client.state == READING)
		{
			printLog("ℹ️ Client closed connection mid-chunked-body", BYEL);
			client.state = CLOSING;
		}
		return;
	}

	bool hasContentEncoding = headersLower.find("content-encoding:") != std::string::npos;
	bool hasIdentityEncoding = hasHeaderToken(headersLower, "content-encoding:", "identity");
	if (hasContentEncoding && !hasIdentityEncoding)
	{
		client.status = 415;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}

	// Always parse Content-Length from headers after receiving them.
	if (!parseContentLengthValue(headersLower, client.contentLength))
	{
		printLog("🚨 Invalid Content-Length", RED);
		client.status = 400;
		client.keepAlive = false;
		client.state = WRITING;
		return;
	}

	// Extract the path from the request line to determine location-specific maxBodySize
	// Format: "METHOD /path HTTP/VERSION"
	std::string requestLine = client.readBuffer.substr(0, requestLineEnd);
	size_t firstSpace = requestLine.find(' ');
	size_t secondSpace = requestLine.find(' ', firstSpace + 1);
	size_t effectiveMaxUploadSize = maxUploadSize; // Default to global size
	
	if (firstSpace != std::string::npos && secondSpace != std::string::npos && firstSpace < secondSpace)
	{
		std::string path = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
		// Find query string and remove it
		size_t queryPos = path.find('?');
		if (queryPos != std::string::npos)
			path = path.substr(0, queryPos);
		
		// Get location-specific maxBodySize by checking configured locations
		// (simplified: check /post_body, /directory, etc.)
		const std::vector<LocationConfig> &locations = server.getLocations();
		for (size_t i = 0; i < locations.size(); ++i)
		{
			const LocationConfig &loc = locations[i];
			const std::string &locPath = loc.getPath();
			// Simple path matching: if request path starts with location path
			if (!locPath.empty() && path.find(locPath) == 0 && loc.getMaxBodySizeFlag())
			{
				effectiveMaxUploadSize = loc.getMaxBodySize();
				break;
			}
		}
	}

	// If declared body is larger than configured upload limit, fail early.
	if (client.contentLength > effectiveMaxUploadSize)
	{
		printLog("🚨 Content-Length exceeds maximum limit", RED);
		client.state = WRITING;
		client.keepAlive = false;
		client.status = 413;
		return;
	}

	// Overflow-safe guard before computing (headerSize + effectiveMaxUploadSize).
	// Prevents wrapping size_t on pathological configuration/input combinations.
	if (headerSize > std::numeric_limits<size_t>::max() - effectiveMaxUploadSize)
	{
		printLog("🚨 Request size overflow guard triggered", RED);
		client.keepAlive = false;
		client.status = 413;
		client.state = WRITING;
		return;
	}

	// Total request budget = bounded headers + bounded body.
	// This prevents unbounded growth even after headers are complete.
	size_t maxRequestSize = headerSize + effectiveMaxUploadSize;
	if (client.readBuffer.size() > maxRequestSize)
	{
		printLog("🚨 Request exceeds configured total size", RED);
		client.keepAlive = false;
		client.status = 413;
		client.state = WRITING;
		return;
	}

	{
		size_t bodySize = client.readBuffer.size() - headerSize;
		// Once enough body bytes have arrived, the request is complete and can be parsed.
		if (bodySize >= client.contentLength)
			client.state = PROCESSING;
		else if (peerClosed)
		{
			// Peer sent FIN before the full body arrived — request is incomplete.
			printLog("ℹ️ Client closed connection with incomplete request body", BYEL);
			client.state = CLOSING;
		}
	}
}