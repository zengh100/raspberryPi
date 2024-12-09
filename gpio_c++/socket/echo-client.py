# echo-client.py

import socket

#HOST = "127.0.0.1"  # The server's hostname or IP address
HOST = "192.168.2.48"
PORT = 8081 #8080 #65432  # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b"Hello, world")
    data = s.recv(1024)

print(f"Received {data!r}")
