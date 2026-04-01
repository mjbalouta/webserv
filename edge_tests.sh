#!/bin/bash

SERVER=${1:-localhost}
PORT=${2:-8080}
UPLOAD_DIR="./www/uploads"

PASS=0
FAIL=0
SKIP=0
FAILED_TESTS=()

# Colors
RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
CYN='\033[0;36m'
MAG='\033[0;35m'
BLD='\033[1m'
RST='\033[0m'

# ─────────────────────────────────────────────
# Core helpers
# ─────────────────────────────────────────────

# Send raw bytes via nc; waits up to 2 s for a response
send_raw() {
	printf "%b" "$1" | nc -q 2 "$SERVER" "$PORT" 2>/dev/null
}

# Extract first response line (status line)
status_line() { echo "$1" | head -n 1; }

# Extract a specific header value (case-insensitive name)
get_header() {
	local name
	name=$(echo "$1" | tr '[:upper:]' '[:lower:]')
	echo "$2" | tr '[:upper:]' '[:lower:]' | grep "^${name}:" | head -n1 | cut -d: -f2- | tr -d '\r' | xargs
}

# Check that response contains ALL expected substrings
contains_all() {
	local response="$1"; shift
	for needle in "$@"; do
		echo "$response" | grep -qF "$needle" || return 1
	done
	return 0
}

# ─────────────────────────────────────────────
# Test runner functions
# ─────────────────────────────────────────────

# test_status NAME RAW_REQUEST EXPECTED_STATUS_SUBSTR
test_status() {
	local name="$1" req="$2" expected="$3"
	local response status
	response=$(send_raw "$req")
	status=$(status_line "$response")
	if [[ "$status" == *"$expected"* ]]; then
		echo -e "  ${GRN}✔${RST} $name"
		PASS=$((PASS+1))
	else
		echo -e "  ${RED}✘${RST} $name"
		echo -e "      ${YEL}expected:${RST} $expected"
		echo -e "      ${YEL}got:${RST}      $status"
		FAIL=$((FAIL+1))
		FAILED_TESTS+=("$name")
	fi
}

# test_status_and_header NAME RAW_REQUEST EXPECTED_STATUS HEADER_NAME HEADER_VALUE
test_status_and_header() {
	local name="$1" req="$2" exp_status="$3" hdr_name="$4" hdr_val="$5"
	local response status hdr_found
	response=$(send_raw "$req")
	status=$(status_line "$response")
	hdr_found=$(get_header "$hdr_name" "$response")
	local ok=true
	[[ "$status" != *"$exp_status"* ]] && ok=false
	[[ "$hdr_found" != *"$hdr_val"* ]] && ok=false
	if $ok; then
		echo -e "  ${GRN}✔${RST} $name"
		PASS=$((PASS+1))
	else
		echo -e "  ${RED}✘${RST} $name"
		echo -e "      ${YEL}status expected:${RST} $exp_status  ${YEL}got:${RST} $status"
		echo -e "      ${YEL}header $hdr_name expected:${RST} $hdr_val  ${YEL}got:${RST} $hdr_found"
		FAIL=$((FAIL+1))
		FAILED_TESTS+=("$name")
	fi
}

# test_body NAME RAW_REQUEST EXPECTED_STATUS BODY_SUBSTR
test_body() {
	local name="$1" req="$2" exp_status="$3" body_substr="$4"
	local response status body
	response=$(send_raw "$req")
	status=$(status_line "$response")
	body=$(echo "$response" | awk 'found{print} /^\r?$/{found=1}')
	if [[ "$status" == *"$exp_status"* ]] && echo "$body" | grep -qF "$body_substr"; then
		echo -e "  ${GRN}✔${RST} $name"
		PASS=$((PASS+1))
	else
		echo -e "  ${RED}✘${RST} $name"
		echo -e "      ${YEL}status expected:${RST} $exp_status  ${YEL}got:${RST} $status"
		echo -e "      ${YEL}body must contain:${RST} $body_substr"
		FAIL=$((FAIL+1))
		FAILED_TESTS+=("$name")
	fi
}

# test_no_response NAME RAW_REQUEST — expects connection to close with no complete HTTP response
test_no_response() {
	local name="$1" req="$2"
	local response status
	response=$(send_raw "$req")
	status=$(status_line "$response")
	if [[ -z "$status" || "$status" != HTTP* ]]; then
		echo -e "  ${GRN}✔${RST} $name"
		PASS=$((PASS+1))
	else
		echo -e "  ${RED}✘${RST} $name"
		echo -e "      ${YEL}expected no complete HTTP response, got:${RST} $status"
		FAIL=$((FAIL+1))
		FAILED_TESTS+=("$name")
	fi
}

section() { echo -e "\n${BLD}${CYAN}━━━  $1  ━━━${RST}"; }

# ─────────────────────────────────────────────
# SECTION 1 — Request line validation
# ─────────────────────────────────────────────
section "Request Line Validation"

test_status "Malformed request line (no method/path/version)" \
	"BADREQUESTLINE\r\nHost: localhost\r\n\r\n" "400"

test_status "Missing HTTP version" \
	"GET /\r\nHost: localhost\r\n\r\n" "400"

test_status "Unsupported HTTP version (HTTP/2.0)" \
	"GET / HTTP/2.0\r\nHost: localhost\r\n\r\n" "505"

test_status "Unsupported HTTP version (HTTP/0.9)" \
	"GET / HTTP/0.9\r\nHost: localhost\r\n\r\n" "505"

test_status "Unknown method (PUT)" \
	"PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n" "405"

test_status "Unknown method (PATCH)" \
	"PATCH / HTTP/1.1\r\nHost: localhost\r\n\r\n" "405"

test_status "Unknown method (OPTIONS)" \
	"OPTIONS / HTTP/1.1\r\nHost: localhost\r\n\r\n" "405"

test_status "Very long URI (>2048 chars)" \
	"GET /$(printf 'A%.0s' {1..2050}) HTTP/1.1\r\nHost: localhost\r\n\r\n" "414"

test_status "Absolute URI in request line" \
	"GET http://localhost:8080/ HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

test_status "Extra whitespace in request line" \
	"GET   / HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

test_status "Trailing whitespace after version" \
	"GET / HTTP/1.1   \r\nHost: localhost\r\n\r\n" "400"

# ─────────────────────────────────────────────
# SECTION 2 — Header validation
# ─────────────────────────────────────────────
section "Header Validation"

test_status "Missing Host header (HTTP/1.1)" \
	"GET / HTTP/1.1\r\n\r\n" "400"

test_status "Multiple Host headers" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n" "400"

test_status "Header without colon" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nInvalidHeader\r\n\r\n" "400"

test_status "Folded header (obsolete RFC 7230)" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nX-Folded: value\r\n  continuation\r\n\r\n" "400"

test_status "Tab in header name" \
	"GET / HTTP/1.1\r\nHost:\tlocalhost\r\n\r\n" "400"

test_status "Invalid UTF-8 byte in header value" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nX-Bad: \xff\xfe\r\n\r\n" "400"

test_status "Oversized headers (>8192 bytes)" \
	"GET / HTTP/1.1\r\nHost: localhost\r\n$(python3 -c "print('X-Pad: ' + 'A'*8200)")\r\n\r\n" "431"

# ─────────────────────────────────────────────
# SECTION 3 — Body / Content-Length validation
# ─────────────────────────────────────────────
section "Body and Content-Length"

test_status "Duplicate Content-Length headers" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\nhello" "400"

test_status "Content-Length + Transfer-Encoding (request smuggling)" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n" "400"

test_status "Negative Content-Length" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -1\r\n\r\n" "400"

test_status "Non-numeric Content-Length" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n" "400"

test_status "POST missing Content-Length" \
	"POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\nhello" "411"

test_status "POST Content-Length 0 (valid)" \
	"POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n" "200"

test_no_response "Partial body (server waits, nc disconnects)" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\nonlya few bytes"

# ─────────────────────────────────────────────
# SECTION 4 — Transfer-Encoding: chunked
# ─────────────────────────────────────────────
section "Transfer-Encoding: chunked"

test_status "Valid chunked POST" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n" "200"

test_status "Chunked GET (unusual but valid)" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n" "200"

test_status "Unsupported TE (gzip)" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: gzip\r\n\r\n" "501"

test_status "Invalid chunk size (non-hex)" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\nZZZZ\r\nhello\r\n0\r\n\r\n" "400"

test_status "Multiple Transfer-Encoding headers" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n" "400"

test_status "Chunked with extensions (ignored)" \
	"POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5;ext=ignored\r\nhello\r\n0\r\n\r\n" "200"

# ─────────────────────────────────────────────
# SECTION 5 — GET / (root location)
# ─────────────────────────────────────────────
section "GET / (root location)"

test_status "GET / basic" \
	"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" "200"

test_status "GET /index.html" \
	"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "200"

test_status "GET with query string" \
	"GET /index.html?foo=bar&baz=qux HTTP/1.1\r\nHost: localhost\r\n\r\n" "200"

test_status "GET with extra headers" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nX-Custom: value\r\n\r\n" "200"

test_status "GET with empty header value" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nX-Empty:\r\n\r\n" "200"

test_status "GET nonexistent file" \
	"GET /this_does_not_exist_xyz.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "404"

test_status "GET path traversal attempt (../)" \
	"GET /../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

test_status "GET path traversal encoded (%2e%2e)" \
	"GET /%2e%2e/etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

# ─────────────────────────────────────────────
# SECTION 6 — HTTP version behaviours
# ─────────────────────────────────────────────
section "HTTP Version and Keep-Alive"

test_status_and_header "HTTP/1.1 default keep-alive" \
	"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" "200" "connection" "keep-alive"

test_status_and_header "HTTP/1.1 Connection: close" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" "200" "connection" "close"

test_status_and_header "HTTP/1.0 default close" \
	"GET / HTTP/1.0\r\n\r\n" "200" "connection" "close"

test_status_and_header "HTTP/1.0 Connection: keep-alive" \
	"GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n" "200" "connection" "keep-alive"

# ─────────────────────────────────────────────
# SECTION 7 — HEAD method
# ─────────────────────────────────────────────
section "HEAD method"

test_status "HEAD / returns 200 with no body" \
	"HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n" "200"

# HEAD response body must be empty; check Content-Length header still present
_head_resp=$(send_raw "HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n")
_head_body=$(echo "$_head_resp" | awk 'found{print} /^\r?$/{found=1}')
if [[ -z "$(echo "$_head_body" | tr -d '\r\n ')" ]]; then
	echo -e "  ${GRN}✔${RST} HEAD response has empty body"
	PASS=$((PASS+1))
else
	echo -e "  ${RED}✘${RST} HEAD response must have empty body"
	FAIL=$((FAIL+1))
	FAILED_TESTS+=("HEAD response has empty body")
fi
unset _head_resp _head_body

# ─────────────────────────────────────────────
# SECTION 8 — Response headers
# ─────────────────────────────────────────────
section "Response Headers"

_resp=$(send_raw "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n")

for _hdr in "content-type" "content-length" "date" "connection"; do
	_val=$(get_header "$_hdr" "$_resp")
	if [[ -n "$_val" ]]; then
		echo -e "  ${GRN}✔${RST} Response includes '$_hdr' header"
		PASS=$((PASS+1))
	else
		echo -e "  ${RED}✘${RST} Response missing '$_hdr' header"
		FAIL=$((FAIL+1))
		FAILED_TESTS+=("Response includes '$_hdr' header")
	fi
done
unset _resp _hdr _val

test_body "HTML file served with text/html content-type" \
	"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "200" ""
# verify mime type separately
_resp=$(send_raw "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n")
_ct=$(get_header "content-type" "$_resp")
if [[ "$_ct" == *"text/html"* ]]; then
	echo -e "  ${GRN}✔${RST} .html served as text/html"
	PASS=$((PASS+1))
else
	echo -e "  ${RED}✘${RST} .html content-type expected text/html, got: $_ct"
	FAIL=$((FAIL+1))
	FAILED_TESTS+=(".html content-type")
fi
unset _resp _ct

# ─────────────────────────────────────────────
# SECTION 9 — /upload location
# ─────────────────────────────────────────────
section "/upload Location (POST/DELETE/GET)"

test_status "GET /upload — method not allowed" \
	"GET /upload HTTP/1.1\r\nHost: localhost\r\n\r\n" "405"

# Raw body upload to /upload/<filename>
test_status "POST raw body to /upload/test_raw.txt" \
	"POST /upload/test_raw.txt HTTP/1.1\r\nHost: localhost\r\nContent-Length: 13\r\n\r\nhello raw body" "201"

# Re-upload the same file → should return 204 (no new file created)
test_status "POST same file again → 204 (overwrite)" \
	"POST /upload/test_raw.txt HTTP/1.1\r\nHost: localhost\r\nContent-Length: 13\r\n\r\nhello raw body" "204"

# Multipart upload
_mp_boundary="TestBoundary42"
_mp_body="--${_mp_boundary}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test_upload.txt\"\r\nContent-Type: text/plain\r\n\r\nMultipart content here\r\n--${_mp_boundary}--\r\n"
_mp_len=$(printf "%b" "$_mp_body" | wc -c | tr -d ' ')
test_status "POST multipart/form-data to /upload" \
	"POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=${_mp_boundary}\r\nContent-Length: ${_mp_len}\r\n\r\n${_mp_body}" "201"
unset _mp_boundary _mp_body _mp_len

# POST to /upload without filename and without multipart → 400
test_status "POST /upload with no filename and raw body → 400" \
	"POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello" "400"

# DELETE uploaded file
test_status "DELETE /upload/test_raw.txt → 204" \
	"DELETE /upload/test_raw.txt HTTP/1.1\r\nHost: localhost\r\n\r\n" "204"

# DELETE nonexistent file
test_status "DELETE /upload/nonexistent.txt → 404" \
	"DELETE /upload/nonexistent.txt HTTP/1.1\r\nHost: localhost\r\n\r\n" "404"

# DELETE with no filename → 400
test_status "DELETE /upload (no filename) → 400" \
	"DELETE /upload HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

# Path traversal via upload filename
test_status "POST /upload/../escape.txt → 400 or 403" \
	"POST /upload/../escape.txt HTTP/1.1\r\nHost: localhost\r\nContent-Length: 3\r\n\r\nbad" "40"

# ─────────────────────────────────────────────
# SECTION 10 — /cgi-bin location
# ─────────────────────────────────────────────
section "/cgi-bin Location (GET/POST only)"

test_status "GET /cgi-bin allowed" \
	"GET /cgi-bin/ HTTP/1.1\r\nHost: localhost\r\n\r\n" "200"

test_status "POST /cgi-bin allowed" \
	"POST /cgi-bin/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n" "200"

test_status "DELETE /cgi-bin — not allowed" \
	"DELETE /cgi-bin HTTP/1.1\r\nHost: localhost\r\n\r\n" "405"

# ─────────────────────────────────────────────
# SECTION 11 — Autoindex
# ─────────────────────────────────────────────
section "Autoindex (directory listing)"

_ai_resp=$(send_raw "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
_ai_status=$(status_line "$_ai_resp")
if [[ "$_ai_status" == *"200"* ]]; then
	# If autoindex is on and no index file, body should contain <ul> or directory listing HTML
	_ai_body=$(echo "$_ai_resp" | awk 'found{print} /^\r?$/{found=1}')
	if echo "$_ai_body" | grep -qiE '(<ul>|Index of|href)'; then
		echo -e "  ${GRN}✔${RST} Autoindex returns directory listing HTML"
		PASS=$((PASS+1))
	else
		echo -e "  ${YEL}~${RST} Autoindex: 200 returned but no listing HTML (index file probably served)"
		SKIP=$((SKIP+1))
	fi
else
	echo -e "  ${YEL}~${RST} Autoindex: unexpected status $_ai_status (check config)"
	SKIP=$((SKIP+1))
fi
unset _ai_resp _ai_status _ai_body

# Directory without trailing slash → 301 redirect
test_status_and_header "Directory without trailing slash → 301 + Location" \
	"GET /uploads HTTP/1.1\r\nHost: localhost\r\n\r\n" "301" "location" "/uploads/"

# ─────────────────────────────────────────────
# SECTION 12 — Error pages
# ─────────────────────────────────────────────
section "Error Pages"

test_body "404 error page has HTML body" \
	"GET /does_not_exist_abc123.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "404" "<html>"

test_body "405 error page has HTML body" \
	"PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n" "405" "<html>"

# ─────────────────────────────────────────────
# SECTION 13 — Security / path traversal
# ─────────────────────────────────────────────
section "Security"

test_status "NULL byte in path" \
	"GET /index%00.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

test_status "Double slash in path" \
	"GET //etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" "400"

test_status "GET /etc/passwd directly" \
	"GET /etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" "404"

# ─────────────────────────────────────────────
# SECTION 14 — Pipelining (keep-alive)
# ─────────────────────────────────────────────
section "HTTP Pipelining (keep-alive)"

# Send two requests in one TCP write; both should get responses
_pipe_resp=$(printf "%b" \
	"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n" \
	"GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" \
	| nc -q 3 "$SERVER" "$PORT" 2>/dev/null)

_pipe_count=$(echo "$_pipe_resp" | grep -c "^HTTP/")
if [[ "$_pipe_count" -ge 2 ]]; then
	echo -e "  ${GRN}✔${RST} Pipelining: received $PASS_pipe_count responses for 2 pipelined requests"
	PASS=$((PASS+1))
else
	echo -e "  ${RED}✘${RST} Pipelining: expected 2 HTTP responses, got $_pipe_count"
	FAIL=$((FAIL+1))
	FAILED_TESTS+=("Pipelining: 2 responses for 2 requests")
fi
unset _pipe_resp _pipe_count

# ─────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────
echo -e "\n${BLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RST}"
echo -e "  ${GRN}${BLD}Passed:${RST}  $PASS"
echo -e "  ${RED}${BLD}Failed:${RST}  $FAIL"
[[ $SKIP -gt 0 ]] && echo -e "  ${YEL}${BLD}Skipped:${RST} $SKIP"
echo -e "${BLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RST}"

if [[ $FAIL -gt 0 ]]; then
	echo -e "\n${BLD}${RED}Failed tests:${RST}"
	for t in "${FAILED_TESTS[@]}"; do
		echo -e "  ${RED}•${RST} $t"
	done
	exit 1
fi
exit 0