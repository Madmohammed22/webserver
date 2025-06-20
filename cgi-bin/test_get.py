#!/usr/bin/env python3
import os

print("Content-Type: text/plain\n")
print(f"GET Request Received")
print(f"Query String: {os.environ.get('QUERY_STRING', '')}")
print(f"User Agent: {os.environ.get('SERVER_PROTOCOL', '')}")
