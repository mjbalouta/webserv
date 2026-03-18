#!/bin/bash

SERVER=localhost
PORT=8080

PASS=0
FAIL=0
FAILED_TESTS=()

function test_case() {
    local name="$1"
    local req="$2"
    local expected="$3"
    response=$(echo -e "$req" | nc -q 1 $SERVER $PORT)
    status=$(echo "$response" | head -n 1)
    if [[ "$status" == *"$expected"* ]]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        FAILED_TESTS+=("$name | expected: $expected | got: $status")
    fi
}

# Original tests
test_case "Malformed Request Line" "BADREQUESTLINE\r\nHost: localhost\r\n\r\n" "400 Bad Request"
test_case "Missing Host Header (HTTP/1.1)" "GET / HTTP/1.1\r\n\r\n" "400 Bad Request"
test_case "Duplicate Content-Length" "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\nhello" "400 Bad Request"
test_case "Unsupported Method" "PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n" "405 Method Not Allowed"
test_case "Unsupported HTTP Version" "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n" "505 HTTP Version Not Supported"
test_case "Malformed Header Line" "GET / HTTP/1.1\r\nHost localhost\r\n\r\n" "400 Bad Request"
test_case "Partial Request (Incomplete Body)" "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nabc" ""
test_case "Request Smuggling (CL + TE)" "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n" "400 Bad Request"
test_case "Oversized Headers" "GET / HTTP/1.1\r\nHost: localhost\r\n$(printf 'X-Fill: %.0sA' {1..9000})\r\n\r\n" "431 Request Header Fields Too Large"
test_case "Absolute URI in Request Line" "GET http://localhost:8080/ HTTP/1.1\r\nHost: localhost\r\n\r\n" "400 Bad Request"

# Additional tests
test_case "GET with Query String" "GET /index.html?foo=bar&baz=qux HTTP/1.1\r\nHost: localhost\r\n\r\n" "200 OK"
test_case "POST with valid Content-Length" "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello" "200 OK"
test_case "POST missing Content-Length" "POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\nhello" "411 Length Required"
test_case "POST with Content-Length 0" "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n" "200 OK"
test_case "GET with extra headers" "GET / HTTP/1.1\r\nHost: localhost\r\nX-Test: value\r\nX-Another: test\r\n\r\n" "200 OK"
test_case "GET with Connection: keep-alive" "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n" "200 OK"
test_case "GET with Connection: close" "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n" "200 OK"
test_case "GET with folded header" "GET / HTTP/1.1\r\nHost: localhost\r\nX-Folded: value\r\n another line\r\n\r\n" "400 Bad Request"
test_case "GET with invalid UTF-8 in header" "GET / HTTP/1.1\r\nHost: localhost\r\nX-Bad: \xFF\xFE\r\n\r\n" "400 Bad Request"
LONG_URI=$(printf '/%.0sA' {1..2050})
test_case "GET with very long URI" "GET $LONG_URI HTTP/1.1\r\nHost: localhost\r\n\r\n" "414 Request-URI Too Long"
test_case "GET with multiple Host headers" "GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n" "400 Bad Request"
test_case "GET with whitespace in method" "GET   / HTTP/1.1\r\nHost: localhost\r\n\r\n" "400 Bad Request"
test_case "GET with tab in header name" "GET / HTTP/1.1\r\nHost:\tlocalhost\r\n\r\n" "400 Bad Request"
test_case "GET with empty header value" "GET / HTTP/1.1\r\nHost: localhost\r\nX-Empty:\r\n\r\n" "200 OK"
test_case "POST with chunked TE" "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n" "200 OK"
test_case "GET with absolute path and port" "GET http://localhost:8080/index.html HTTP/1.1\r\nHost: localhost\r\n\r\n" "400 Bad Request"
test_case "GET with HTTP/1.0 and keep-alive" "GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n" "200 OK"
test_case "GET with HTTP/1.0 and no Host" "GET / HTTP/1.0\r\n\r\n" "200 OK"
test_case "GET with trailing whitespace" "GET /index.html HTTP/1.1   \r\nHost: localhost\r\n\r\n" "400 Bad Request"
test_case "GET with invalid header (no colon)" "GET / HTTP/1.1\r\nHost: localhost\r\nInvalidHeader\r\n\r\n" "400 Bad Request"

echo -e "\n=== Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
if [ $FAIL -gt 0 ]; then
    echo -e "\nFailed tests:"
    for fail in "${FAILED_TESTS[@]}"; do
        echo "- $fail"
    done
fi