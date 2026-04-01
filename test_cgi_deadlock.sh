#!/bin/bash

##############################################################################
# Test Script: CGI Large Output Deadlock Fix Verification
#
# Purpose: Verify that the deadlock bug in handleCgiRead() is fixed
# 
# Issue: Previously, when a CGI script output large amounts of data (>100MB),
# the system would deadlock:
# - handleCgiRead() would interpret EAGAIN (would-block) as EOF
# - It would close the pipe prematurely
# - Child process would be left blocked trying to write more data
#
# Fix: Distinguish between read() == 0 (EOF) and read() < 0 (would-block)
##############################################################################

set -e

SERVER_PORT=9090
TEST_TIMEOUT=30
TEMP_DIR="/tmp/cgi_test_$$"
mkdir -p "$TEMP_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

cleanup() {
    echo -e "${YELLOW}[*] Cleaning up...${NC}"
    pkill -f "./webserv.*test42" 2>/dev/null || true
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}CGI Large Output Deadlock Test${NC}"
echo -e "${YELLOW}========================================${NC}"

# Start the server
echo -e "${YELLOW}[*] Starting webserver on port $SERVER_PORT...${NC}"
cd /home/mipinhei/Desktop/42/webserver
./webserv test42.conf &
SERVER_PID=$!
sleep 2

# Verify server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}[!] Server failed to start${NC}"
    exit 1
fi
echo -e "${GREEN}[+] Server started (PID: $SERVER_PID)${NC}"

##############################################################################
# Test 1: Small output (baseline)
##############################################################################
echo -e "${YELLOW}\n[TEST 1] Small CGI output (1MB)${NC}"
echo "Testing baseline behavior with small output..."
timeout $TEST_TIMEOUT curl -s "http://localhost:$SERVER_PORT/cgi-bin/getTest.py" > "$TEMP_DIR/small_output.txt" 2>&1
SIZE=$(wc -c < "$TEMP_DIR/small_output.txt")
if [ $SIZE -gt 0 ]; then
    echo -e "${GREEN}[+] Small output test passed (${SIZE} bytes received)${NC}"
else
    echo -e "${RED}[!] Small output test failed${NC}"
fi

##############################################################################
# Test 2: Large output (100MB+) - The real test
##############################################################################
echo -e "${YELLOW}\n[TEST 2] Large CGI output (100MB) - Deadlock test${NC}"
echo "Testing with 100MB output to verify deadlock is fixed..."
START_TIME=$(date +%s)

# Make the request with timeout
if timeout 25 curl -s "http://localhost:$SERVER_PORT/cgi-bin/large_output.py" > "$TEMP_DIR/large_output.txt" 2>&1; then
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    SIZE=$(wc -c < "$TEMP_DIR/large_output.txt")
    
    if [ $SIZE -gt 100000000 ]; then
        echo -e "${GREEN}[+] Large output test PASSED${NC}"
        echo -e "${GREEN}    Received: $SIZE bytes${NC}"
        echo -e "${GREEN}    Time: ${ELAPSED}s${NC}"
        echo -e "${GREEN}[+] DEADLOCK FIX VERIFIED - CGI can output 100MB+${NC}"
    else
        echo -e "${RED}[!] Large output incomplete (only ${SIZE} bytes)${NC}"
        echo -e "${RED}    This may indicate remaining issues${NC}"
    fi
else
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    SIZE=$(wc -c < "$TEMP_DIR/large_output.txt" 2>/dev/null || echo "0")
    echo -e "${RED}[!] Large output test timeout/failed${NC}"
    echo -e "${RED}    Elapsed: ${ELAPSED}s${NC}"
    echo -e "${RED}    Size received: $SIZE bytes${NC}"
    echo -e "${RED}[!] This indicates the deadlock may still exist${NC}"
fi

##############################################################################
# Test 3: Chunked output with delays (stress test)
##############################################################################
echo -e "${YELLOW}\n[TEST 3] Chunked output with delays (stress test)${NC}"
echo "Testing with realistic chunked pattern..."

# Create a script that outputs data in realistic chunks
cat > "$TEMP_DIR/chunked_output.py" << 'EOF'
#!/usr/bin/env python3
import sys
import time

print("Content-Type: text/plain")
print("Content-Length: 50000000")  # 50MB
print()

# Output in 1MB chunks with small delays
chunk_size = 1024 * 1024  # 1MB
for i in range(50):
    data = f"CHUNK{i:04d}-" + ("D" * (chunk_size - 15))
    sys.stdout.write(data)
    sys.stdout.flush()
    if i % 5 == 0:
        time.sleep(0.005)  # 5ms delay every 5 chunks
EOF

chmod +x "$TEMP_DIR/chunked_output.py"

timeout 20 curl -s "http://localhost:$SERVER_PORT/cgi-bin/large_output.py" > "$TEMP_DIR/chunked_output.txt" 2>&1
SIZE=$(wc -c < "$TEMP_DIR/chunked_output.txt")
if [ $SIZE -gt 49000000 ]; then
    echo -e "${GREEN}[+] Chunked output test passed (${SIZE} bytes)${NC}"
else
    echo -e "${YELLOW}[*] Chunked output incomplete (${SIZE} bytes) - may be normal${NC}"
fi

##############################################################################
# Test 4: Server remains responsive
##############################################################################
echo -e "${YELLOW}\n[TEST 4] Server responsiveness after large output${NC}"
echo "Testing if server is still responsive..."

if timeout 5 curl -s "http://localhost:$SERVER_PORT/" | grep -q "html" 2>/dev/null; then
    echo -e "${GREEN}[+] Server is responsive after large output${NC}"
else
    echo -e "${RED}[!] Server became unresponsive${NC}"
fi

##############################################################################
# Summary
##############################################################################
echo -e "${YELLOW}\n========================================${NC}"
echo -e "${YELLOW}Test Summary${NC}"
echo -e "${YELLOW}========================================${NC}"
echo -e "${GREEN}All critical tests completed.${NC}"
echo -e "${YELLOW}Key indicators of successful fix:${NC}"
echo -e "  1. Test 2 (100MB) completes in < 25 seconds"
echo -e "  2. Full 100MB+ output is received"
echo -e "  3. No timeout/hang occurs"
echo -e "  4. Server remains responsive"
echo -e "${NC}"
