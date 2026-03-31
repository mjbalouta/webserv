#!/usr/bin/env python3
"""
CGI script that outputs a large amount of data (100MB+).
This is designed to trigger the deadlock bug in handleCgiRead().

The output is split into chunks with newlines to ensure the
child process doesn't block too quickly due to the large size.
"""

import sys
import time

def main():
    print("Content-Type: text/plain")
    print("Content-Length: 104857600")  # 100MB
    print()
    
    # Output 100MB of data in 1MB chunks
    # This forces multiple read() calls in handleCgiRead()
    chunk_size = 1024 * 1024  # 1MB
    total_chunks = 100
    
    for i in range(total_chunks):
        # Each chunk: 1MB of data
        data = f"CHUNK_{i:04d}_" + ("X" * (chunk_size - 20)) + "\n"
        sys.stdout.write(data)
        sys.stdout.flush()
        
        # Add small delay to simulate realistic output pattern
        # This increases chances of hitting the read() would-block case
        if i % 10 == 0:
            time.sleep(0.01)

if __name__ == "__main__":
    main()
