import socket
import time

def send_request_character_by_character(host, port, path, delay=1):
    """
    Sends an HTTP GET request character by character to a web server.
    
    Args:
        host (str): The target host (e.g., 'example.com')
        port (int): The target port (typically 80 for HTTP)
        path (str): The path to request (e.g., '/index.html')
        delay (float): Delay between sending each character in seconds
    """
    # Construct the HTTP request
    request = f"GET {path} HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\n\r\n"
    
    # Create a socket connection
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    
    print(f"Sending request to {host}:{port}{path}")
    print("Request content:")
    print(request)
    print("Sending character by character...")
    
    # Send each character with a delay
    for char in request:
        s.send(char.encode('utf-8'))
        print(f"Sent: {repr(char)}")
        time.sleep(0.3)
    
    # Receive and print the response
    print("\nReceiving response...")
    response = b""
    while True:
        data = s.recv(1024)
        if not data:
            break
        response += data
    
    print("\nResponse received:")
    print(response.decode('utf-8'))
    
    s.close()

if __name__ == "__main__":
    # Example usage
    target_host = "localhost"
    target_port = 8000
    target_path = "/"
    
    send_request_character_by_character(target_host, target_port, target_path, delay=0.05)
