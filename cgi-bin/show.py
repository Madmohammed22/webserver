#!/usr/bin/env python3
import os, sys

print("HTTP/1.1 200 OK")
print("Content-Type: text/plain")
print()
if os.environ['REQUEST_METHOD'] != 'POST':
    print("Error: Only POST requests allowed")
    sys.exit()

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_data = sys.stdin.read(content_length)
print(f"RAW POST DATA:\n{post_data}")
