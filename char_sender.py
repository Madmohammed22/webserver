import socket
import time

# Configuration
host = "localhost"  # Change to your target host
port = 8000             # 443 for HTTPS (needs ssl), 80 for HTTP
path = "/"            # Path to request
delay = 1             # Delay (seconds) between each character
timeout = 10          # Socket timeout in seconds

# Request to send
request = f"GET {path} HTTP/1.1\r\n\r\n"

def send_char_by_char(host, port, request, delay, timeout):
    try:
        # Create a socket
        with socket.create_connection((host, port), timeout=timeout) as sock:
            sock.settimeout(timeout)

            print(f"[+] Connected to {host}:{port}")
            print("[*] Sending request character by character...")

            for char in request:
                sock.send(char.encode())
                print(f"Sent: {repr(char)}")
                time.sleep(delay)  # Delay between characters

            print("[*] All characters sent. Waiting for response...")

            response = b""
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    print("[!] Socket timeout reached while receiving response.")
                    break

            print("[*] Response received:")
            print(response.decode(errors="ignore"))

    except socket.timeout:
        print("[!] Connection timed out.")
    except Exception as e:
        print(f"[!] Error occurred: {e}")

# Run it
send_char_by_char(host, port, request, delay, timeout)

