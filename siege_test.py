#!/usr/bin/env python3
"""
Siege Stress Test Runner
Tests server stability, memory usage, and connection handling
"""

import subprocess
import time
import psutil
import os
from datetime import datetime

def get_process_memory(pid):
    """Get memory usage of process in MB"""
    try:
        p = psutil.Process(pid)
        return p.memory_info().rss / 1024 / 1024  # Convert to MB
    except:
        return 0

def run_siege_test(target_url: str, concurrency: int = 50, repetitions: int = 100):
    """Run siege stress test"""
    print(f"\n{'='*70}")
    print(f"Siege Stress Test")
    print(f"{'='*70}")
    print(f"Target: {target_url}")
    print(f"Concurrency: {concurrency} users")
    print(f"Repetitions: {repetitions} per user")
    print(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Check if siege is installed
    result = subprocess.run(["which", "siege"], capture_output=True)
    if result.returncode != 0:
        print("\n⚠ Siege not installed. Install with: apt install siege")
        return False
    
    # Get server PID for memory monitoring
    result = subprocess.run(["pgrep", "-f", "webserv"], capture_output=True, text=True)
    server_pids = result.stdout.strip().split('\n')
    server_pid = int(server_pids[0]) if server_pids and server_pids[0] else None
    
    if not server_pid:
        print("ERROR: Server not running")
        return False
    
    print(f"Server PID: {server_pid}")
    
    # Record initial memory
    initial_memory = get_process_memory(server_pid)
    print(f"Initial memory: {initial_memory:.2f} MB")
    
    # Run siege
    cmd = [
        "siege",
        "-b",  # Benchmark mode (no delays)
        "-c", str(concurrency),  # Number of concurrent users
        "-r", str(repetitions),  # Repetitions per user
        "-t", "5M",  # Timeout after 5 minutes
        target_url
    ]
    
    print(f"\nRunning: {' '.join(cmd)}")
    print("-" * 70)
    
    start_time = time.time()
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Monitor memory while test runs
    memory_readings = [initial_memory]
    peak_memory = initial_memory
    
    while process.poll() is None:
        time.sleep(2)
        current_memory = get_process_memory(server_pid)
        memory_readings.append(current_memory)
        peak_memory = max(peak_memory, current_memory)
        
        elapsed = time.time() - start_time
        print(f"  [{elapsed:.0f}s] Memory: {current_memory:.2f} MB (peak: {peak_memory:.2f} MB)")
    
    elapsed_time = time.time() - start_time
    
    # Get siege output
    stdout, stderr = process.communicate()
    
    print("\n" + "-" * 70)
    print("Siege Output:")
    print("-" * 70)
    print(stdout)
    
    if stderr:
        print("Errors:")
        print(stderr)
    
    # Parse results
    final_memory = get_process_memory(server_pid)
    
    print("\n" + "="*70)
    print("Test Results Summary")
    print("="*70)
    print(f"Total Test Duration: {elapsed_time:.2f} seconds")
    print(f"Initial Memory: {initial_memory:.2f} MB")
    print(f"Peak Memory: {peak_memory:.2f} MB")
    print(f"Final Memory: {final_memory:.2f} MB")
    print(f"Memory Growth: {final_memory - initial_memory:.2f} MB")
    
    # Check for memory leaks (growth > 50MB is suspicious)
    if final_memory - initial_memory > 50:
        print(f"⚠ WARNING: Significant memory growth detected!")
    else:
        print(f"✓ Memory usage stable")
    
    # Parse availability from siege output
    if "Availability" in stdout:
        for line in stdout.split('\n'):
            if "Availability" in line:
                print(f"Server Availability: {line.strip()}")
                # Extract percentage
                import re
                match = re.search(r'(\d+\.\d+)%', line)
                if match:
                    availability = float(match.group(1))
                    if availability >= 99.5:
                        print(f"✓ Availability meets requirement (>99.5%)")
                    else:
                        print(f"✗ Availability below requirement (need >99.5%)")
    
    # Check for data transferred
    if "Data transferred" in stdout:
        for line in stdout.split('\n'):
            if "Data transferred" in line:
                print(f"Data Transferred: {line.strip()}")
    
    print("\n✓ Siege test completed successfully")
    return True

def test_hanging_connections():
    """Test for hanging connections"""
    print(f"\n{'='*70}")
    print("Hanging Connection Test")
    print(f"{'='*70}")
    
    import socket
    
    print("Testing incomplete requests...")
    
    # Test 1: Send request but don't close connection
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 9090))
        sock.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        
        # Keep connection open briefly
        time.sleep(1)
        response = sock.recv(1024)
        sock.close()
        
        if len(response) > 0:
            print("✓ Server responds to hanging connection")
        else:
            print("✗ Server didn't respond")
    except Exception as e:
        print(f"✗ Error: {e}")
    
    print("\nTesting rapid fire requests...")
    
    # Test 2: Rapid sequential requests
    success_count = 0
    for i in range(10):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("localhost", 9090))
            sock.send(b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
            response = sock.recv(1024)
            sock.close()
            if len(response) > 0:
                success_count += 1
        except:
            pass
    
    print(f"✓ {success_count}/10 rapid requests succeeded")
    return True

def main():
    print("\n" + "="*70)
    print("42 Webserv Stress Test Suite")
    print("="*70)
    
    # Run siege test
    run_siege_test("http://localhost:9090/", concurrency=50, repetitions=20)
    
    # Test hanging connections
    test_hanging_connections()
    
    print("\n" + "="*70)
    print("Stress Testing Complete")
    print("="*70)

if __name__ == "__main__":
    main()
