#!/usr/bin/env python3
import time

print("Content-Type: text/plain\n")

try:
    while True:
        print("CGI script is still running...")
        print(f"Current time: {time.ctime()}")
        print("-" * 40)
        time.sleep(1)
except:
    print("\nScript was terminated")
