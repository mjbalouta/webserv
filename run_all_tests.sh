#!/bin/bash
# 42 Webserv Evaluation Test Suite Runner
# Runs all evaluation tests from the 42 evaluation sheet

set -e

BOLD='\033[1m'
GREEN='\033[92m'
BLUE='\033[94m'
RESET='\033[0m'

echo -e "\n${BOLD}${BLUE}42 Webserv Evaluation Test Suite${RESET}"
echo -e "${BOLD}================================${RESET}\n"

cd /home/mipinhei/Desktop/Common_Core/webserv_git

# Check server is built
if [ ! -f "./webserv" ]; then
    echo "Building webserv..."
    make clean
    make
    echo ""
fi

# Function to run test
run_test() {
    local name=$1
    local cmd=$2
    echo -e "\n${BOLD}$name${RESET}"
    echo "================================"
    $cmd
}

# Kill any existing servers
pkill -f webserv 2>/dev/null || true
sleep 1

# Start server on port 8080 with default.conf
echo "Starting server on port 8080..."
./webserv default.conf > /tmp/webserv.log 2>&1 &
sleep 2

# Test 1: Basic Evaluation Tests (test on port 8080)
run_test "1. Basic Evaluation Tests (Status Codes, Routes, Methods)" \
    "python3 evaluation_tester.py 8080"

# Test 2: Advanced Tests
run_test "2. Advanced Tests (Multi-port, CGI, Error Handling)" \
    "python3 advanced_tests.py"

# Test 3: Concurrent Stress
run_test "3. Concurrent Connection Test (5/10/50/100 simultaneous)" \
    "python3 concurrent_test.py"

# Test 4: Siege Stress Testing (if available)
if command -v siege &> /dev/null; then
    run_test "4. Siege Stress Test (Availability & Memory)" \
        "python3 siege_test.py"
else
    echo -e "\n${BOLD}4. Siege Stress Test (Not Installed)${RESET}"
    echo "================================"
    echo "Install siege with: sudo apt-get install siege"
    echo "Then run: python3 siege_test.py"
fi

# Summary
echo -e "\n${BOLD}${GREEN}================================${RESET}"
echo -e "${BOLD}${GREEN}Test Suite Complete!${RESET}"
echo -e "${BOLD}${GREEN}================================${RESET}\n"

echo "Test Results:"
echo "  ✓ evaluation_tester.py     - Basic configuration tests"
echo "  ✓ advanced_tests.py        - Multi-server, CGI, error tests"
echo "  ✓ concurrent_test.py       - Load testing (5-100 concurrent)"
echo "  ✓ siege_test.py            - Stress & memory testing"
echo ""
echo "Full Report: EVALUATION_REPORT.md"
echo ""
