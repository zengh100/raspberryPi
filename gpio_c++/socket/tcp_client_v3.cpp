#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 8081;
constexpr int BUFFER_SIZE = 1024;
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // Convert IPv4 and IPv6 addresses from text to binary form
    //if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
    //if (inet_pton(AF_INET, "192.168.2.48", &serv_addr.sin_addr) <= 0)
    if (inet_pton(AF_INET, "192.168.2.48", &serv_addr.sin_addr) <= 0) //192.168.1.74
    {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }
    // Connect to the server
    //Upon successful completion, connect() shall return 0; 
    //otherwise, -1 shall be returned and errno set to indicate the error.
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }
    std::cout << "connected to server\n";
    std::string hello = "Hello from client";
    send(sock, hello.c_str(), hello.size(), 0);
    std::cout << "message sent: " << hello << std::endl;
    ssize_t valread = read(sock, buffer, BUFFER_SIZE);
    std::cout << "Received: " << buffer << ", valread=" << valread << std::endl;

    std::string input;
    while(1)
    {
        //wait for use input console
        input.clear();//empty the string
        std::cout << "type your command\n";
        std::cin >> input;
        
        send(sock, input.c_str(), input.size(), 0);
        std::cout << "message sent: " << hello << std::endl;

	if (input == "quit") break;

        //prepare to receive from server
        buffer[0] = '\0'; //empty buffer
        valread = read(sock, buffer, BUFFER_SIZE);
        buffer[valread] =  '\0';
        std::cout << "Received: " << buffer << ", valread=" << valread << std::endl;

    }

    // Close the socket
    close(sock);
    return 0;
}
