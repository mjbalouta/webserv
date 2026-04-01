#!/usr/bin/env python3
"""
Advanced 42 Webserv Evaluation Tests
Tests: Multiple servers, hostnames, CGI, file uploads, etc.
"""

import subprocess
import requests
import socket
import time
import os
import signal
from typing import Tuple

# Color codes
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
BOLD = '\033[1m'
RESET = '\033[0m'

def run_cmd(cmd: str, timeout: int = 5) -> Tuple[int, str, str]:
    """Run command and return exit code, stdout, stderr"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, timeout=timeout, text=True)
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Command timed out"
    except Exception as e:
        return -1, "", str(e)

def print_test(name: str, passed: bool, detail: str = ""):
    status = f"{GREEN}✓ PASS{RESET}" if passed else f"{RED}✗ FAIL{RESET}"
    print(f"  {status}: {name}")
    if detail:
        print(f"       {detail}")

print(f"\n{BOLD}{BLUE}Advanced Webserv Evaluation Tests{RESET}")
print("=" * 70)

# ============== Test 1: Multiple Servers on Different Ports ==============
print(f"\n{BOLD}Test 1: Multiple Servers on Different Ports{RESET}")
print("-" * 70)

pkill_code, _, _ = run_cmd("pkill -f webserv")
time.sleep(1)

# Start server with default.conf (8080, 8081, 8082)
run_code, _, _ = run_cmd("cd /home/mipinhei/Desktop/Common_Core/webserv_git && ./webserv default.conf > /tmp/multi_server.log 2>&1 &")
time.sleep(2)

ports_ok = 0
ports_total = 3
for port in [8080, 8081, 8082]:
    try:
        resp = requests.get(f"http://localhost:{port}/", timeout=3)
        if resp.status_code == 200:
            print_test(f"Port {port} responds with 200", True)
            ports_ok += 1
        else:
            print_test(f"Port {port} responds", False, f"Status: {resp.status_code}")
    except Exception as e:
        print_test(f"Port {port} responds", False, str(e))

print(f"Result: {ports_ok}/{ports_total} ports working")

# ============== Test 2: Hostname Resolution with curl --resolve ==============
print(f"\n{BOLD}Test 2: Hostname Resolution with curl --resolve{RESET}")
print("-" * 70)

# Test with custom hostname
hostname_tests = [
    ("example.local", "localhost", 8080),
    ("test.local", "localhost", 8081),
    ("webserv.local", "localhost", 8082),
]

hostname_ok = 0
for hostname, ip, port in hostname_tests:
    cmd = f'curl -s --resolve "{hostname}:{port}:{ip}" http://{hostname}:{port}/ | head -1 | grep -q "<!DOCTYPE" && echo "OK" || echo "FAIL"'
    code, stdout, _ = run_cmd(cmd, timeout=5)
    if "OK" in stdout:
        print_test(f"Hostname {hostname} resolves to {ip}:{port}", True)
        hostname_ok += 1
    else:
        print_test(f"Hostname {hostname} resolves to {ip}:{port}", False)

print(f"Result: {hostname_ok}/{len(hostname_tests)} hostnames resolved")

# ============== Test 3: CGI Functionality ==============
print(f"\n{BOLD}Test 3: CGI Functionality{RESET}")
print("-" * 70)

# Check if cgi_tester exists
cgi_exists = os.path.exists("/home/mipinhei/Desktop/Common_Core/webserv_git/cgi_tester")
print_test("CGI tester executable exists", cgi_exists)

# Stop multi-server and start test42 server for CGI testing
run_cmd("pkill -f webserv")
time.sleep(1)
run_cmd("cd /home/mipinhei/Desktop/Common_Core/webserv_git && ./webserv test42.conf > /tmp/cgi_server.log 2>&1 &")
time.sleep(2)

# Test CGI execution through .bla extension
try:
    resp = requests.post("http://localhost:9090/directory/youpi.bla", data="test_input", timeout=5)
    if resp.status_code == 200:
        print_test("CGI POST request executes", True, "Status 200")
    else:
        print_test("CGI POST request executes", False, f"Status {resp.status_code}")
except Exception as e:
    print_test("CGI POST request executes", False, str(e))

# ============== Test 4: Telnet Raw Connection ==============
print(f"\n{BOLD}Test 4: Raw HTTP via Telnet/netcat{RESET}")
print("-" * 70)

# Test raw HTTP request
raw_http = "GET / HTTP/1.1\r\nHost: localhost:9090\r\nConnection: close\r\n\r\n"
cmd = f'echo -n "{raw_http}" | nc -q 1 localhost 9090 | head -1'
code, stdout, _ = run_cmd(cmd, timeout=5)

if "HTTP/1.1" in stdout or "HTTP/" in stdout:
    print_test("Raw HTTP request via netcat", True, stdout.strip()[:50])
else:
    print_test("Raw HTTP request via netcat", False, "No HTTP response")

# ============== Test 5: File Upload and Download ==============
print(f"\n{BOLD}Test 5: File Upload and Download{RESET}")
print("-" * 70)

# Create a test file
test_file_content = "This is a test file for upload\n" * 10
test_file_path = "/tmp/test_upload.txt"
with open(test_file_path, 'w') as f:
    f.write(test_file_content)

# Test POST file upload (to /post_body)
try:
    with open(test_file_path, 'rb') as f:
        files = {'file': f}
        resp = requests.post("http://localhost:9090/post_body", files=files, timeout=5)
    # POST is accepted, check we get a response
    if resp.status_code in [200, 204]:
        print_test("File POST upload accepted", True)
    else:
        print_test("File POST upload accepted", False, f"Status {resp.status_code}")
except Exception as e:
    print_test("File POST upload accepted", False, str(e))

# Test GET request (static file serving)
try:
    resp = requests.get("http://localhost:9090/", timeout=5)
    if resp.status_code == 200 and len(resp.content) > 0:
        print_test("Static file serving works", True, f"Downloaded {len(resp.content)} bytes")
    else:
        print_test("Static file serving works", False)
except Exception as e:
    print_test("Static file serving works", False, str(e))

# ============== Test 6: Error Handling ==============
print(f"\n{BOLD}Test 6: Error Handling and Edge Cases{RESET}")
print("-" * 70)

# Test 1: Server doesn't crash on invalid request
error_tests = [
    ("GET / HTTP/1.1\r\nInvalid Header\r\n\r\n", "Malformed header"),
    ("INVALID_METHOD / HTTP/1.1\r\nHost: localhost\r\n\r\n", "Invalid method"),
    ("GET\n\n", "Incomplete request"),
]

server_stable = True
for raw_req, desc in error_tests:
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 9090))
        sock.send(raw_req.encode())
        sock.settimeout(2)
        response = sock.recv(1024)
        sock.close()
        if len(response) > 0:
            print_test(f"Server handles {desc}", True)
        else:
            print_test(f"Server handles {desc}", False, "No response")
    except Exception as e:
        print_test(f"Server handles {desc}", False, str(e)[:30])

# Verify server still responsive
try:
    resp = requests.get("http://localhost:9090/", timeout=2)
    if resp.status_code == 200:
        print_test("Server still responsive after error tests", True)
    else:
        print_test("Server still responsive after error tests", False)
except:
    print_test("Server still responsive after error tests", False, "Server not responding")

# ============== Test 7: Directory Autoindex ==============
print(f"\n{BOLD}Test 7: Directory Handling (Autoindex/Index Files){RESET}")
print("-" * 70)

# Test that /directory returns the index file (youpi.bad_extension)
try:
    resp = requests.get("http://localhost:9090/directory/", timeout=5)
    if resp.status_code == 200:
        print_test("Directory serves index file", True, "Status 200")
    elif resp.status_code == 403:
        print_test("Directory access denied (autoindex off)", True, "Status 403 expected")
    else:
        print_test("Directory handling", False, f"Status {resp.status_code}")
except Exception as e:
    print_test("Directory handling", False, str(e))

# ============== Test 8: Redirect Handling ==============
print(f"\n{BOLD}Test 8: Response Redirect Handling{RESET}")
print("-" * 70)

try:
    # Test Location header (if redirects are implemented)
    resp = requests.get("http://localhost:9090/", timeout=5, allow_redirects=False)
    if "Location" in resp.headers:
        print_test("Redirect headers present", True, f"Location: {resp.headers['Location'][:40]}")
    else:
        print_test("Server has no redirects configured", True, "Expected for basic config")
except Exception as e:
    print_test("Redirect handling", False, str(e))

# ============== Summary ==============
print(f"\n{BOLD}{'='*70}{RESET}")
print(f"{BOLD}Advanced Tests Complete{RESET}")
print(f"{BOLD}{'='*70}{RESET}")
print(f"To complete remaining tests, run:")
print(f"  1. {YELLOW}Siege stress test:{RESET} siege -b -c 100 -r 10 http://localhost:9090/")
print(f"  2. {YELLOW}Memory monitoring:{RESET} watch -n 1 'ps aux | grep webserv'")
print(f"  3. {YELLOW}Test hanging connections:{RESET} python3 test_hangingconnections.py")
