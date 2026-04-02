#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r\n")
print("CGI Environment Variables:\n")
for key, value in sorted(os.environ.items()):
    print(f"{key}={value}")
