#include "ServerManager.hpp"

/**
 * @brief Validates Transfer-Encoding lines and reports whether they are exactly "chunked".
 * @param headersLower Lowercased header lines (without request line).
 * @param hasTransferEncoding Output flag indicating presence of Transfer-Encoding header.
 * @param isChunkedOnly Output flag indicating only supported TE values are present.
 * @return the error code.
 */
int ServerManager::parseTransferEncodingHeader(const std::string &headersLower, bool &hasTransferEncoding, bool &isChunkedOnly)
{
	hasTransferEncoding = false;
	isChunkedOnly = false;
	const std::string headerName = "transfer-encoding:";

	// loop through each header line.
	for (size_t lineStart = 0; lineStart < headersLower.size(); )
	{
		// find the end of the current header line.
		size_t lineEnd = headersLower.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = headersLower.size();

		// if this line starts with "transfer-encoding:"
		if (lineEnd > lineStart
			&& headersLower.compare(lineStart, headerName.size(), headerName) == 0)
		{
			// if has more then 1 transfer-encoding, reject
			if (hasTransferEncoding)
				return 1; //Malformed: multiple TE
			hasTransferEncoding = true;

			// get the value part after "transfer-encoding:"
			std::string value = headersLower.substr(lineStart + headerName.size(), lineEnd - (lineStart + headerName.size()));
			value = trimSpaces(value);
			if (value.empty())
				return 1; //Malformed, empty value

			// parse comma-separated tokens in the value
			bool seenChunked = false;
			size_t tokenStart = 0;
			while (tokenStart <= value.size())
			{
				// find the next comma
				size_t commaPos = value.find(',', tokenStart);
				size_t tokenEnd = (commaPos == std::string::npos) ? value.size() : commaPos;
				// get and trim the token
				std::string token = value.substr(tokenStart, tokenEnd - tokenStart);
				token = trimSpaces(token);
				if (token.empty())
					return 1; //Malformed, empty token
				// only chunked,reject any other value
				if (token != "chunked")
					return 2; //Unsuported TE
				// reject if has more then 1 chunked
				if (seenChunked)
					return 1; //Malformed, duplicated
				seenChunked = true;

				// if has no more commas, break
				if (commaPos == std::string::npos)
					break;
				tokenStart = commaPos + 1;
			}

			// if no chunk, reject
			if (!seenChunked)
				return 1; //Malformed, its not  chunked
			isChunkedOnly = true;
		}

		// if is the last line, break
		if (lineEnd == headersLower.size())
			break;
		// start of the next line
		lineStart = lineEnd + 2;
	}

	// is valid
	return 0;
}

void ServerManager::decodeChunked(ClientSession &client, size_t maxUploadSize)
{
	client.state = READING;

	size_t headerEnd = client.readBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return; // Wait for complete headers.

	size_t bodyStart = headerEnd + 4; // Body starts after header terminator.
	std::string body = client.readBuffer.substr(bodyStart); // Extract body bytes.
	std::string decodedBody; // Will hold the decoded chunked payload.
	size_t cursor = 0; // Cursor tracks position in body.

	for (;;)
	{
		size_t lineEnd = body.find("\r\n", cursor);
		if (lineEnd == std::string::npos)
			return; // Wait for complete chunk size line.

		std::string sizeLine = body.substr(cursor, lineEnd - cursor); // Get chunk size line.
		size_t extensionPos = sizeLine.find(';');
		if (extensionPos != std::string::npos)
			sizeLine = sizeLine.substr(0, extensionPos); // Ignore chunk extensions.
		sizeLine = trimSpaces(sizeLine); // Remove whitespace.
		if (sizeLine.empty())
		{
			client.status = 400; // Empty chunk size line is invalid.
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		size_t chunkSize = 0;
		std::istringstream iss(sizeLine);
		iss >> std::hex >> chunkSize;
		if (iss.fail()) {
			client.status = 400;
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}
		/* // Parse chunk size as hexadecimal.
		for (size_t i = 0; i < sizeLine.size(); ++i)
		{
			unsigned char ch = static_cast<unsigned char>(sizeLine[i]);
			int value;
			if (ch >= '0' && ch <= '9')
				value = ch - '0';
			else if (ch >= 'a' && ch <= 'f')
				value = 10 + (ch - 'a');
			else if (ch >= 'A' && ch <= 'F')
				value = 10 + (ch - 'A');
			else
			{
				client.status = 400; // Invalid hex digit.
				client.keepAlive = false;
				client.state = WRITING;
				return;
			}

			// Prevent overflow.
			if (chunkSize > (std::numeric_limits<size_t>::max() - static_cast<size_t>(value)) / 16)
			{
				client.status = 400;
				client.keepAlive = false;
				client.state = WRITING;
				return;
			}
			chunkSize = chunkSize * 16 + static_cast<size_t>(value);
		} */

		cursor = lineEnd + 2; // Move cursor past chunk size line.

		if (chunkSize == 0)
		{
			// Last chunk: look for trailer terminator.
			size_t trailerEnd = body.find("\r\n", cursor);
			if (trailerEnd == std::string::npos)
				return; // Wait for complete trailers.

			size_t consumedBodyBytes = trailerEnd + 2;
			std::string remaining = body.substr(consumedBodyBytes); // Any pipelined requests after chunked body.
			std::string headersPart = client.readBuffer.substr(0, bodyStart); // Preserve headers.
			client.contentLength = decodedBody.size(); // Set decoded body size.
			client.readBuffer = headersPart + decodedBody + remaining; // Replace buffer with decoded body.
			client.state = PROCESSING; // Ready to process request.
			return;
		}

		// Wait for full chunk data and trailing CRLF.
		if (cursor + chunkSize + 2 > body.size())
			return;

		// Enforce max upload size.
		if (decodedBody.size() > maxUploadSize - chunkSize)
		{
			client.status = 413; // Payload Too Large.
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}

		decodedBody.append(body, cursor, chunkSize); // Append chunk data.
		cursor += chunkSize; // Move cursor past chunk data.

		// Validate chunk terminator.
		if (body.compare(cursor, 2, "\r\n") != 0)
		{
			client.status = 400; // Missing chunk CRLF.
			client.keepAlive = false;
			client.state = WRITING;
			return;
		}
		cursor += 2; // Move cursor past chunk CRLF.
	}
}
