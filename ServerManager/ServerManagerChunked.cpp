#include "ServerManager.hpp"

/**
 * @brief Validates Transfer-Encoding lines and reports whether they are exactly "chunked".
 */
int ServerManager::parseTransferEncodingHeader(const std::string &headersLower, bool &hasTransferEncoding, bool &isChunkedOnly)
{
	hasTransferEncoding = false;
	isChunkedOnly = false;
	const std::string headerName = "transfer-encoding:";

	for (size_t lineStart = 0; lineStart < headersLower.size(); )
	{
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = headersLower.size();

		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, headerName.size(), headerName) == 0)
		{
			if (hasTransferEncoding)
				return 1;
			hasTransferEncoding = true;

			std::string value = headersLower.substr(lineStart + headerName.size(), lineEnd - (lineStart + headerName.size()));
			value = trimSpaces(value);
			if (value.empty())
				return 1;

			bool seenChunked = false;
			size_t tokenStart = 0;
			while (tokenStart <= value.size())
			{
				size_t commaPos = value.find(',', tokenStart);
				size_t tokenEnd = (commaPos == std::string::npos) ? value.size() : commaPos;
				std::string token = value.substr(tokenStart, tokenEnd - tokenStart);
				token = trimSpaces(token);
				if (token.empty())
					return 1;
				if (token != "chunked")
					return 2;
				if (seenChunked)
					return 1;
				seenChunked = true;
				if (commaPos == std::string::npos)
					break;
				tokenStart = commaPos + 1;
			}
			if (!seenChunked)
				return 1;
			isChunkedOnly = true;
		}

		if (lineEnd == headersLower.size())
			break;
		lineStart = lineEnd + 2;
	}
	return 0;
}

/**
 * @brief Incrementally decodes a chunked transfer-encoded body.
 *
 * KEY DESIGN: this function resumes exactly where it stopped on the previous
 * call.  Three ClientSession fields survive across calls:
 *
 *   chunkedBodyStart    - absolute offset in readBuffer where the chunked
 *                         body begins (set once when headers are complete).
 *   chunkedCursor       - body-relative offset of the NEXT chunk-size line
 *                         to decode.  Advanced only after a full chunk has
 *                         been consumed (data + trailing CRLF).  When data
 *                         is incomplete the cursor stays at the start of the
 *                         current chunk-size line so we re-parse it next call.
 *   chunkedDecodedBody  - accumulates decoded payload; appended chunk-by-chunk.
 *
 * When the terminal chunk (size 0) is fully buffered, readBuffer is rebuilt
 * as:  <original headers>  +  <decoded body>  +  <any pipelined remainder>
 * and state is set to PROCESSING.
 */
void ServerManager::decodeChunked(ClientSession &client, size_t maxUploadSize)
{
	client.state = READING;

	// On the very first call for this request, locate the body start.
	if (client.chunkedBodyStart == 0)
	{
		size_t headerEnd = client.readBuffer.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
			return;
		client.chunkedBodyStart = headerEnd + 4;
		// chunkedCursor and chunkedDecodedBody are already 0/"" from construction/reset.
	}

	const std::string &buf = client.readBuffer;
	const size_t bodyStart = client.chunkedBodyStart;

	for (;;)
	{
		// Save cursor at the top of each iteration.
		// If data is incomplete we leave chunkedCursor == iterStart so the
		// next call re-parses this chunk-size line (safe; parsing is idempotent).
		size_t iterStart = client.chunkedCursor;
		size_t absPos    = bodyStart + iterStart;

		if (absPos >= buf.size()) {
			return;
		}

		// Find the CRLF terminating the chunk-size line.
		size_t lineEnd = buf.find("\r\n", absPos);
		if (lineEnd == std::string::npos) {
			//printLog("[CHUNKED] Waiting for chunk-size CRLF (cursor " + itostr(iterStart) + ")", BYEL);
			return;
		}

		// Parse size token (strip optional chunk extensions after ';').
		std::string sizeLine = buf.substr(absPos, lineEnd - absPos);
		size_t extPos = sizeLine.find(';');
		if (extPos != std::string::npos)
			sizeLine = sizeLine.substr(0, extPos);
		sizeLine = trimSpaces(sizeLine);

		if (sizeLine.empty())
		{
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		size_t chunkSize = 0;
		std::istringstream iss(sizeLine);
		iss >> std::hex >> chunkSize;
		if (iss.fail())
		{
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}
		std::string garbage;
		if (iss >> garbage)
		{
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		// Body-relative offset of the first data byte (past chunk-size line + CRLF).
		size_t dataStart = (lineEnd + 2) - bodyStart;

		// --- Terminal chunk (chunkSize == 0) ---
		if (chunkSize == 0)
		{
			if (bodyStart + dataStart + 2 > buf.size()) {
				//printLog("[CHUNKED] Waiting for terminal CRLF", BYEL);
				// cursor stays at iterStart
				return;
			}

			size_t consumedBodyBytes;
			if (buf.compare(bodyStart + dataStart, 2, "\r\n") == 0)
			{
				consumedBodyBytes = dataStart + 2;
			}
			else
			{
				size_t trailerEnd = buf.find("\r\n\r\n", bodyStart + dataStart);
				if (trailerEnd == std::string::npos) {
					//printLog("[CHUNKED] Waiting for trailer terminator", BYEL);
					return;
				}
				consumedBodyBytes = (trailerEnd + 4) - bodyStart;
			}

			// Rebuild readBuffer: headers + decoded body + pipelined remainder.
			std::string remaining   = buf.substr(bodyStart + consumedBodyBytes);
			std::string headersPart = buf.substr(0, bodyStart);
			client.contentLength    = client.chunkedDecodedBody.size();
			client.readBuffer       = headersPart + client.chunkedDecodedBody + remaining;

			//printLog("[CHUNKED] Done. Decoded body: " + itostr(client.contentLength) + " bytes", BYEL);

			// Reset incremental state for future requests on this keep-alive connection.
			client.chunkedDecodedBody.clear();
			client.chunkedCursor    = 0;
			client.chunkedBodyStart = 0;
			client.chunkedDecoded   = true;
			client.state            = PROCESSING;
			return;
		}

		// --- Normal chunk: need all data + trailing CRLF buffered ---
		if (bodyStart + dataStart + chunkSize + 2 > buf.size())
		{
			//printLog("[CHUNKED] Waiting for chunk data (cursor " + itostr(iterStart) +
				//", need " + itostr(chunkSize) + " + 2 bytes, have " +
				//itostr(buf.size() > bodyStart + dataStart ? buf.size() - bodyStart - dataStart : 0) + ")", BYEL);
			// Leave chunkedCursor == iterStart; re-parse size line next call.
			return;
		}

		// Enforce upload size limit.
		if (client.chunkedDecodedBody.size() + chunkSize > maxUploadSize)
		{
			client.status = 413;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		// Validate trailing CRLF.
		if (buf.compare(bodyStart + dataStart + chunkSize, 2, "\r\n") != 0)
		{
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		// Consume the chunk and advance cursor past data + trailing CRLF.
		//printLog("[CHUNKED] Chunk ok cursor=" + itostr(iterStart) + " size=" + itostr(chunkSize), BYEL);
		client.chunkedDecodedBody.append(buf, bodyStart + dataStart, chunkSize);
		client.chunkedCursor = dataStart + chunkSize + 2;
	}
}