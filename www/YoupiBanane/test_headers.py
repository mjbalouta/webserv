#!/usr/bin/env python3
import sys
import os

# Read input from stdin
body = sys.stdin.read()
body_size = len(body)

# Output headers with special characters and newlines
print("Content-Type: text/plain")
print("X-Body-Size: " + str(body_size))
print("X-Custom-Header: value-with:;colons;;;")
print()
print(body, end='')
