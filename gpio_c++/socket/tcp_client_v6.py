import socket

PORT = 8081
BUFFER_SIZE = 1024
SERVER_IP = "192.168.2.48"  # Change as needed

def main():
    try:
        # Create a socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except socket.error as e:
        print(f"Socket creation error: {e}")
        return -1

    try:
        # Connect to the server
        sock.connect((SERVER_IP, PORT))
    except socket.error as e:
        print(f"Connection Failed: {e}")
        return -1

    print("Connected to server. Waiting for message from server")

    try:
        # Receive initial message
        data = sock.recv(BUFFER_SIZE)
        print(f"Received: {data.decode()}, valread={len(data)}")

        while True:
            # Get input from user
            input_str = input("Type your command:\n")

            # Send input to server
            sock.sendall(input_str.encode())
            print(f"Message sent: {input_str}")

            if input_str == "quit":
                break

            # Receive server response
            data = sock.recv(BUFFER_SIZE)
            print(f"Received: {data.decode()}, valread={len(data)}")

    finally:
        sock.close()
        print("Socket closed")

    return 0

if __name__ == "__main__":
    main()
