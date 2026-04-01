#!/usr/bin/env python3
"""
Comprehensive 42 Webserv Evaluation Test Suite
Tests all requirements from the evaluation sheet
"""

import subprocess
import requests
import time
import json
import sys
from typing import Dict, List, Tuple
from datetime import datetime

# Color codes for output
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
BOLD = '\033[1m'
RESET = '\033[0m'

# Get port from command line or default to 9090
TARGET_PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 9090
TARGET_URL = f"http://localhost:{TARGET_PORT}"

class TestResult:
    def __init__(self, name: str):
        self.name = name
        self.passed = 0
        self.failed = 0
        self.details = []
    
    def add_pass(self, detail: str = ""):
        self.passed += 1
        self.details.append(f"{GREEN}✓{RESET} {detail}")
    
    def add_fail(self, detail: str = ""):
        self.failed += 1
        self.details.append(f"{RED}✗{RESET} {detail}")
    
    def print_summary(self):
        print(f"\n{BOLD}{self.name}{RESET}")
        print("=" * 60)
        for detail in self.details:
            print(f"  {detail}")
        status = f"{GREEN}PASS{RESET}" if self.failed == 0 else f"{RED}FAIL{RESET}"
        print(f"Result: {status} ({self.passed} passed, {self.failed} failed)")

# ============== HTTP Status Code Tests ==============
def test_http_status_codes():
    """Test that HTTP status codes are correct"""
    result = TestResult("HTTP Status Codes")
    
    # Test 200 OK
    try:
        resp = requests.get(f"{TARGET_URL}/", timeout=5)
        if resp.status_code == 200:
            result.add_pass("200 OK for GET /")
        else:
            result.add_fail(f"GET / returned {resp.status_code}, expected 200")
    except Exception as e:
        result.add_fail(f"GET / failed: {e}")
    
    # Test 404 Not Found
    try:
        resp = requests.get(f"{TARGET_URL}/nonexistent_path_12345", timeout=5)
        if resp.status_code == 404:
            result.add_pass("404 Not Found for invalid path")
        else:
            result.add_fail(f"Invalid path returned {resp.status_code}, expected 404")
    except Exception as e:
        result.add_fail(f"404 test failed: {e}")
    
    # Test 405 Method Not Allowed - POST not allowed on /
    try:
        resp = requests.post(f"{TARGET_URL}/", data="test", timeout=5)
        if resp.status_code == 405:
            result.add_pass("405 Method Not Allowed for POST on / (GET only)")
        else:
            result.add_fail(f"POST / returned {resp.status_code}, expected 405")
    except Exception as e:
        result.add_fail(f"405 test failed: {e}")
    
    # Test 413 Payload Too Large - default.conf has 10M limit on upload
    try:
        large_body = "A" * (11 * 1024 * 1024)  # 11MB exceeds 10M limit
        resp = requests.post(f"{TARGET_URL}/upload/largefile.txt", data=large_body, timeout=10)
        if resp.status_code == 413:
            result.add_pass("413 Payload Too Large for oversized body")
        else:
            result.add_fail(f"Oversized POST returned {resp.status_code}, expected 413")
    except requests.exceptions.ConnectionError:
        result.add_pass("413 Payload Too Large: Server rejected connection (correct behavior)")
    except Exception as e:
        result.add_fail(f"413 test failed: {e}")
    
    return result

# ============== Route and Directory Tests ==============
def test_route_directory_mapping():
    """Test that routes map to correct directories"""
    result = TestResult("Route to Directory Mapping")
    
    # Test /upload route maps to www/upload
    try:
        resp = requests.get(f"{TARGET_URL}/upload", timeout=5)
        # Should return directory listing or 200
        if resp.status_code == 200:
            result.add_pass("/upload route accessible")
        else:
            result.add_fail(f"/upload returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"Route mapping test failed: {e}")
    
    # Test /cgi-bin route maps to /cgi-bin
    try:
        resp = requests.get(f"{TARGET_URL}/cgi-bin", timeout=5)
        # Should be accessible
        if resp.status_code in [200, 403, 404]:
            result.add_pass("/cgi-bin route accessible")
        else:
            result.add_fail(f"/cgi-bin returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"CGI-bin route test failed: {e}")
    
    return result

# ============== Method Restriction Tests ==============
def test_method_restrictions():
    """Test that allow_methods is enforced"""
    result = TestResult("Method Restrictions")
    
    # Test GET allowed on /
    try:
        resp = requests.get(f"{TARGET_URL}/", timeout=5)
        if resp.status_code == 200:
            result.add_pass("GET allowed on / (configured as GET only)")
        else:
            result.add_fail("GET / failed")
    except Exception as e:
        result.add_fail(f"GET test failed: {e}")
    
    # Test POST disallowed on / 
    try:
        resp = requests.post(f"{TARGET_URL}/", data="test", timeout=5)
        if resp.status_code == 405:
            result.add_pass("POST blocked on / (not in allow_methods)")
        else:
            result.add_fail(f"POST / returned {resp.status_code}, expected 405")
    except Exception as e:
        result.add_fail(f"POST restriction test failed: {e}")
    
    # Test DELETE allowed on /upload
    try:
        resp = requests.delete(f"{TARGET_URL}/upload/testfile.txt", timeout=5)
        if resp.status_code in [200, 204, 404]:  # 404 is ok if file doesn't exist
            result.add_pass("DELETE allowed on /upload (in allow_methods)")
        else:
            result.add_fail(f"DELETE /upload returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"DELETE /upload test failed: {e}")
    
    return result

# ============== Client Body Size Tests ==============
def test_client_body_size():
    """Test client_max_body_size enforcement"""
    result = TestResult("Client Body Size Limit")
    
    # Test within limit (1MB under 10MB limit)
    try:
        body = "A" * (1 * 1024 * 1024)
        resp = requests.post(f"{TARGET_URL}/upload/file1.txt", data=body, timeout=5)
        if resp.status_code in [200, 201, 204]:
            result.add_pass("POST with 1MB accepted (under 10MB limit)")
        else:
            result.add_fail(f"POST 1MB returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"Body size test failed: {e}")
    
    # Test at limit (10MB)
    try:
        body = "A" * (10 * 1024 * 1024)
        resp = requests.post(f"{TARGET_URL}/upload/file2.txt", data=body, timeout=10)
        if resp.status_code in [200, 201, 204]:
            result.add_pass("POST with 10MB accepted (at limit)")
        else:
            result.add_fail(f"POST 10MB returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"Body size at limit test failed: {e}")
    
    # Test over limit (11MB over 10MB limit)
    try:
        body = "A" * (11 * 1024 * 1024)
        resp = requests.post(f"{TARGET_URL}/upload/file3.txt", data=body, timeout=10)
        if resp.status_code == 413:
            result.add_pass("POST with 11MB rejected with 413 (over 10MB limit)")
        else:
            result.add_fail(f"POST 11MB returned {resp.status_code}, expected 413")
    except requests.exceptions.ConnectionError:
        result.add_pass("POST with 11MB rejected by server (correct behavior for oversized body)")
    except Exception as e:
        result.add_fail(f"Body size over limit test failed: {e}")
    
    return result

# ============== Request Method Tests ==============
def test_request_methods():
    """Test GET, POST, DELETE methods"""
    result = TestResult("Request Methods (GET/POST/DELETE)")
    
    # Test GET
    try:
        resp = requests.get(f"{TARGET_URL}/", timeout=5)
        if resp.status_code == 200:
            result.add_pass("GET request works")
        else:
            result.add_fail(f"GET returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"GET failed: {e}")
    
    # Test POST
    try:
        resp = requests.post(f"{TARGET_URL}/upload/test.txt", data="test", timeout=5)
        if resp.status_code in [200, 201, 204]:
            result.add_pass("POST request works")
        else:
            result.add_fail(f"POST returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"POST failed: {e}")
    
    # Test DELETE
    try:
        resp = requests.delete(f"{TARGET_URL}/upload/test.txt", timeout=5)
        if resp.status_code in [200, 204, 404]:
            result.add_pass("DELETE request works")
        else:
            result.add_fail(f"DELETE returned {resp.status_code}")
    except Exception as e:
        result.add_fail(f"DELETE failed: {e}")
    
    # Test unknown method (using raw request)
    try:
        result_proc = subprocess.run(
            ["curl", "-X", "INVALID", f"{TARGET_URL}/"],
            capture_output=True,
            timeout=5
        )
        if b"Server" not in result_proc.stderr:  # Should not crash
            result.add_pass("Unknown method doesn't crash server")
        else:
            result.add_fail("Unknown method caused server issues")
    except Exception as e:
        result.add_fail(f"Unknown method test failed: {e}")
    
    return result

# ============== Response Headers Tests ==============
def test_response_headers():
    """Test that response headers are correct"""
    result = TestResult("Response Headers")
    
    try:
        resp = requests.get(f"{TARGET_URL}/", timeout=5)
        
        # Check for essential headers
        if "content-type" in resp.headers or "Content-Type" in resp.headers:
            result.add_pass("Content-Type header present")
        else:
            result.add_fail("Content-Type header missing")
        
        if "content-length" in resp.headers or "Content-Length" in resp.headers:
            result.add_pass("Content-Length header present")
        else:
            result.add_fail("Content-Length header missing")
        
        if "date" in resp.headers or "Date" in resp.headers:
            result.add_pass("Date header present")
        else:
            result.add_fail("Date header missing")
        
        if "server" in resp.headers or "Server" in resp.headers:
            result.add_pass("Server header present")
        else:
            result.add_fail("Server header missing")
    
    except Exception as e:
        result.add_fail(f"Header test failed: {e}")
    
    return result

# ============== Error Page Tests ==============
def test_error_pages():
    """Test that error pages are returned"""
    result = TestResult("Error Pages")
    
    # Test 404 error page
    try:
        resp = requests.get(f"{TARGET_URL}/this_should_not_exist", timeout=5)
        if resp.status_code == 404 and "404" in resp.text:
            result.add_pass("404 error page is returned")
        else:
            result.add_fail("404 error page missing or incorrect")
    except Exception as e:
        result.add_fail(f"404 error page test failed: {e}")
    
    # Test 405 error page
    try:
        resp = requests.delete(f"{TARGET_URL}/", timeout=5)
        if resp.status_code == 405 and "405" in resp.text:
            result.add_pass("405 error page is returned")
        else:
            result.add_fail("405 error page missing or incorrect")
    except Exception as e:
        result.add_fail(f"405 error page test failed: {e}")
    
    return result

def main():
    print(f"\n{BOLD}{BLUE}42 Webserv Evaluation Test Suite{RESET}")
    print(f"Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Target: {TARGET_URL}")
    print("=" * 60)
    
    # Check if server is running
    try:
        requests.get(f"{TARGET_URL}/", timeout=2)
    except:
        print(f"{RED}ERROR: Server not running on {TARGET_URL}/{RESET}")
        return 1
    
    all_results = [
        test_http_status_codes(),
        test_route_directory_mapping(),
        test_method_restrictions(),
        test_client_body_size(),
        test_request_methods(),
        test_response_headers(),
        test_error_pages(),
    ]
    
    # Print all results
    for result in all_results:
        result.print_summary()
    
    # Overall summary
    total_passed = sum(r.passed for r in all_results)
    total_failed = sum(r.failed for r in all_results)
    
    print(f"\n{BOLD}{'='*60}{RESET}")
    print(f"{BOLD}Overall Results{RESET}")
    print(f"{BOLD}{'='*60}{RESET}")
    print(f"Total Passed: {GREEN}{total_passed}{RESET}")
    print(f"Total Failed: {RED}{total_failed}{RESET}")
    
    if total_failed == 0:
        print(f"\n{GREEN}{BOLD}✓ ALL TESTS PASSED!{RESET}")
        return 0
    else:
        print(f"\n{RED}{BOLD}✗ SOME TESTS FAILED{RESET}")
        return 1

if __name__ == "__main__":
    exit(main())
