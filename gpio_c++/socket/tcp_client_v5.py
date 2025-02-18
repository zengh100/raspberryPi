# echo-client.py

import socket

#HOST = "127.0.0.1"  # The server's hostname or IP address
HOST = "192.168.2.48"
PORT = 8081 #8080 #65432  # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print("connected to server Host IP:", HOST, ", Port:", PORT)
    data = s.recv(1024)
    print("Received: ", data)#, ", length=", data.len)
    while(True):
        #s.sendall(b"get8")
        text = input("type command such get8 where 8 is pin number (valid from 0 tp 27, inclusive)\n") 
        s.sendall(text.encode())
        if text == "quit": break
        data = s.recv(1024)
        print("Received: ", data, ", length=", len(data));

    

#print(f"Received {data!r}")

