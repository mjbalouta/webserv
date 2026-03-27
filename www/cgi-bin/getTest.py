#!/usr/bin/env python3
import os
import datetime

method      = os.environ.get("REQUEST_METHOD", "UNKNOWN")
query       = os.environ.get("QUERY_STRING", "")
host        = os.environ.get("HTTP_HOST", "UNKNOWN")
protocol    = os.environ.get("SERVER_PROTOCOL", "UNKNOWN")
script      = os.environ.get("SCRIPT_FILENAME", "UNKNOWN")

# Parse query string into key=value pairs
params = {}
if query:
    for pair in query.split("&"):
        if "=" in pair:
            k, v = pair.split("=", 1)
            params[k] = v
        else:
            params[pair] = ""

name = params.get("name", "stranger")

body = """Hello, {name}!

--- Request Info ---
Method:   {method}
Host:     {host}
Protocol: {protocol}
Script:   {script}
Query:    {query}
Time:     {time}
""".format(
    name=name,
    method=method,
    host=host,
    protocol=protocol,
    script=script,
    query=query if query else "(none)",
    time=datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
)

print("Content-Type: text/plain")
print("Content-Length: " + str(len(body)))
print("")
print(body)