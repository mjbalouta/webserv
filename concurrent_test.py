#!/usr/bin/env python3
"""
Concurrent HTTP request tester for localhost:8080
Tests with 5, 10, 50, and 100 simultaneous requests
"""

import concurrent.futures
import requests
import sys
import time
from datetime import datetime

# Configuration
TARGET_URL = "http://localhost:8080/"
REQUEST_COUNTS = [5, 10, 50, 100]
TIMEOUT = 10  # seconds per request

def make_request(request_num):
    """Make a single HTTP GET request and return result"""
    try:
        start = time.time()
        response = requests.get(TARGET_URL, timeout=TIMEOUT)
        elapsed = time.time() - start
        return {
            "num": request_num,
            "status": response.status_code,
            "elapsed": elapsed,
            "success": response.status_code == 200,
            "error": None
        }
    except Exception as e:
        return {
            "num": request_num,
            "status": None,
            "elapsed": None,
            "success": False,
            "error": str(e)
        }

def run_concurrent_requests(count):
    """Run 'count' concurrent requests and return results"""
    print(f"\n{'='*60}")
    print(f"Testing with {count} concurrent requests")
    print(f"{'='*60}")
    
    start_time = time.time()
    results = []
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=count) as executor:
        futures = [executor.submit(make_request, i) for i in range(count)]
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
    
    total_time = time.time() - start_time
    
    # Sort results by request number for display
    results.sort(key=lambda x: x["num"])
    
    # Calculate statistics
    successful = sum(1 for r in results if r["success"])
    failed = count - successful
    avg_time = sum(r["elapsed"] for r in results if r["elapsed"]) / (count - failed) if (count - failed) > 0 else 0
    
    # Display results
    print(f"\nResults:")
    print(f"  Total Requests: {count}")
    print(f"  Successful: {successful}")
    print(f"  Failed: {failed}")
    print(f"  Total Time: {total_time:.2f}s")
    print(f"  Average Response Time: {avg_time:.3f}s")
    
    # Show details if there are failures
    if failed > 0:
        print(f"\nFailed requests:")
        for r in results:
            if not r["success"]:
                print(f"  Request {r['num']}: {r['error']}")
    else:
        print(f"\n✓ All {count} requests passed!")
    
    return successful == count, results

def main():
    """Main test runner"""
    print(f"Concurrent Request Tester")
    print(f"Target: {TARGET_URL}")
    print(f"Timeout: {TIMEOUT}s per request")
    print(f"Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    all_passed = True
    
    for count in REQUEST_COUNTS:
        passed, _ = run_concurrent_requests(count)
        if not passed:
            all_passed = False
    
    print(f"\n{'='*60}")
    if all_passed:
        print("✓ ALL TESTS PASSED!")
        print(f"{'='*60}")
        return 0
    else:
        print("✗ SOME TESTS FAILED!")
        print(f"{'='*60}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
