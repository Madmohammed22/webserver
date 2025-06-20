#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain\n")
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_data = sys.stdin.read(content_length)
print(f"Received {content_length} bytes:")
print(post_data)
