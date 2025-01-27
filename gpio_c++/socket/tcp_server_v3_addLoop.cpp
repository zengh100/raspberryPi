#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#include <thread>

//Build: g++ -o myServer tcp_server.cpp

constexpr int PORT = 8081;
constexpr int BUFFER_SIZE = 1024;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.2.48");//INADDR_ANY; //192.168.1.74
    address.sin_port = htons(PORT);
    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << std::endl;
    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "connected!" << std::endl;
    ssize_t valread = 0;
    while(1)
    {
    	// Read and echo the received message
        buffer[0] = '\0';
	valread = 0;
        const size_t timeout_ms = 1*60*1000; //size_t equivalent unsigned int
        const size_t sleep_ms = 100;
        size_t elapsed_ms = 0;

        //while (valread <= 0 && elapsed_ms < timeout_ms) // better add a timer to timeout
        while (valread <= 0)
        {
           valread = read(new_socket, buffer, BUFFER_SIZE);
           std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
           elapsed_ms += sleep_ms;
           if (elapsed_ms%2000 == 0)
	           std::cout << '\r' << "elapsed_ms: " << elapsed_ms << std::flush;
           if(elapsed_ms >= timeout_ms)
               break;
        }
        if (valread <= 0) {
           std::cout << "\nelapsed_ms = " << elapsed_ms << ". Timeout\n";
           break;
        }
        buffer[valread] =  '\0';
    	std::cout << "Received: " << buffer << ", valread=" << valread << std::endl;

        //check if "quit" is received
        if(std::string(buffer).compare("quit") == 0) 
        {
            std::cout << "Client quit. Exiting this conversation.\n";
            break;
        }

    	send(new_socket, buffer, valread, 0);
    	std::cout << "Echo message sent" << std::endl;
        //todo: add code to check if client indicating leaving
        //receive "quit" so we can quit or respnd accordingly
    };

    // Close the socket
    close(new_socket);
    close(server_fd);
    return 0;
}
