#include <iostream>
#include <string>
#include <sstream>
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
#include <pi-gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <mutex>

constexpr int PORT = 8081;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAXCLIENTS = 3;

class GPIOHandler {
public:
    static bool process_get(int socket, const std::string& bufString) {
        bool gpioCmd = false;
        int valread = bufString.size();
        std::string pinStr = bufString.substr(3, valread);
        std::cout << "pinStr=" << pinStr << "\n";

        int pin = convertStrToInteger(pinStr);
        if (pin == -1) {
            return false;
        }

        if (checkRange(pin, 0, 27)) {
            gpioCmd = true;
            int n = input_gpio(pin);
            int f = gpio_function(pin);
            int p = (f == 0) ? get_pullupdn(pin) : 0;
            
            std::stringstream ss;
            if (f == 0) {
                ss << "GPIO " << pin << ": Value=" << n << " Function=" << f << " Pull=" << p;
            } else {
                ss << "GPIO " << pin << ": Value=" << n << " Function=" << f;
            }
            
            sendResponse(socket, ss.str());
        }
        return gpioCmd;
    }

    static bool process_set(int socket, const std::string& bufString) {
        std::cout << "process_set: bufString=" << bufString << "\n";
        auto params = split(bufString);
        if(params.size() < 4 || params[0] != "set") {
            return false;
        }

        int pin = convertStrToInteger(params[1]);
        int mode = convertStrToInteger(params[2]);
        int value = convertStrToInteger(params[3]);

        if(!checkRange(pin, 0, 27) || !checkRange(mode, 0, 1) || !checkRange(value, 0, 1)) {
            return false;
        }

        if(mode == 1) {
            setup_gpio(pin, OUTPUT, value);
            output_gpio(pin, value);
        } else {
            setup_gpio(pin, INPUT, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Read back and send response
        int n = input_gpio(pin);
        int f = gpio_function(pin);
        int p = (f == 0) ? get_pullupdn(pin) : 0;
        
        std::stringstream ss;
        if (f == 0) {
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f << " Pull=" << p;
        } else {
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f;
        }

        sendResponse(socket, ss.str());
        return true;
    }

private:
    static std::vector<std::string> split(const std::string& my_str) {
        std::vector<std::string> result;
        std::stringstream s_stream(my_str);
        std::string substr;
        
        while(getline(s_stream, substr, ',')) {
            result.push_back(substr);
        }
        return result;
    }

    static int convertStrToInteger(const std::string& strIn) {
        int i = atoi(strIn.c_str());
        std::stringstream ss;
        ss << i;
        if(strIn != ss.str()) {
            return -1;
        }
        return i;
    }

    static bool checkRange(int v, int vMin, int vMax) {
        return vMin <= v && v <= vMax;
    }

    static void sendResponse(int socket, const std::string& response) {
        send(socket, response.c_str(), response.size(), 0);
        std::cout << "sent: " << response << std::endl;
    }
};

class ClientConnection {
public:
    ClientConnection(int socket, const sockaddr_in& address, int threadId) 
        : socket_(socket), threadId_(threadId) {
        ip_ = inet_ntoa(address.sin_addr);
        port_ = ntohs(address.sin_port);
    }

    void handle() {
        std::string welcome = "Welcome! Type example: get8 or set,8,1,1";
        sendResponse(welcome);
        
        char buffer[BUFFER_SIZE] = {0};
        while(active_) {
            ssize_t valread = readWithTimeout(buffer, BUFFER_SIZE, 60000);
            if (valread <= 0) {
                std::cout << "\nConnection " << threadId_ << " timeout or closed\n";
                break;
            }

            buffer[valread] = '\0';
            std::cout << "Received from " << ip_ << ":" << port_ << ": " << buffer << std::endl;

            if(std::string(buffer) == "quit") {
                sendResponse("Goodbye!");
                //sendResponse("quit");
                break;
            }

            std::string bufString(buffer);
            bool gpioCmd = false;
            
            if(bufString.substr(0,3) == "get") {
                gpioCmd = GPIOHandler::process_get(socket_, bufString);
            } 
            else if(bufString.substr(0,3) == "set") {
                gpioCmd = GPIOHandler::process_set(socket_, bufString);
            }

            if(!gpioCmd) {
                std::string prompt = "Invalid command. Examples: get8 or set,8,1,1";
                sendResponse(prompt);
            }
        }
        closeConnection();
    }

    void closeConnection() {
        if (socket_ != -1) {
            close(socket_);
            socket_ = -1;
            active_ = false;
        }
    }

    std::string getInfo() const {
        return "IP: " + ip_ + ", Port: " + std::to_string(port_) + 
               ", Thread ID: " + std::to_string(threadId_);
    }

    int getThreadId() const { return threadId_; }
    bool isActive() const { return active_; }

private:
    int socket_;
    int threadId_;
    std::string ip_;
    int port_;
    bool active_ = true;

    ssize_t readWithTimeout(char* buffer, size_t size, size_t timeout_ms) {
        const size_t sleep_ms = 100;
        size_t elapsed_ms = 0;
        ssize_t valread = 0;

        while (valread <= 0 && elapsed_ms < timeout_ms) {
            valread = read(socket_, buffer, size);
            if (valread > 0) break;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            elapsed_ms += sleep_ms;
            
            if (elapsed_ms % 2000 == 0) {
                std::cout << '\r' << "Connection " << threadId_ << " waiting: " 
                          << elapsed_ms << "ms" << std::flush;
            }
        }
        return valread;
    }

    void sendResponse(const std::string& response) {
        send(socket_, response.c_str(), response.size(), 0);
        std::cout << "Sent to " << ip_ << ":" << port_ << ": " << response << std::endl;
    }
};

class ConnectionManager {
public:
    ~ConnectionManager() {
        cleanup();
    }

    void addConnection(int socket, const sockaddr_in& address) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connections_.size() >= MAXCLIENTS) {
            std::cerr << "Max clients reached. Rejecting new connection." << std::endl;
            std::string response {"Max clients reached. Connection is rejected."};
            send(socket, response.c_str(), response.size(), 0);
            close(socket);
            return;
        }

        int threadId = nextThreadId_++;
        auto connection = std::make_shared<ClientConnection>(socket, address, threadId);
        connections_[threadId] = connection; //add a pair of <key,value> to the map
        
        //define a lamda (a callable object, or a function pointer)
        threads_.emplace_back([this, connection]() {
            connection->handle();
            removeConnection(connection->getThreadId());
        });
    }

    void removeConnection(int threadId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(threadId);
        if (it != connections_.end()) {
            connections_.erase(it);
        }
    }

    void printConnections() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\nActive Connections (" << connections_.size() << "):\n";
        for (const auto& pair : connections_) {
            std::cout << "- " << pair.second->getInfo() << "\n";
        }
        std::cout << std::endl;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            pair.second->closeConnection();
        }
        connections_.clear();

        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        threads_.clear();
    }

private:
    std::map<int, std::shared_ptr<ClientConnection>> connections_;
    std::vector<std::thread> threads_;
    mutable std::mutex mutex_;
    int nextThreadId_ = 0;
};

class GPIOServer {
public:
    GPIOServer() : server_fd_(-1) {}

    bool start() {
        if (!setupSocket()) {
            return false;
        }

        setup_gpio();
        std::cout << "Server listening on port " << PORT << std::endl;
        return true;
    }

    void run() {
        while (true) {
            sockaddr_in address;
            socklen_t addrlen = sizeof(address);
            int new_socket = accept(server_fd_, (struct sockaddr*)&address, &addrlen);
            
            if (new_socket < 0) {
                perror("accept");
                continue;
            }

            std::cout << "Connected to IP:" << inet_ntoa(address.sin_addr) 
                      << ", port:" << ntohs(address.sin_port) << std::endl;
            
            connectionManager_.addConnection(new_socket, address);
            connectionManager_.printConnections();
        }
    }

    ~GPIOServer() {
        if (server_fd_ != -1) {
            close(server_fd_);
        }
        cleanup_gpio();
    }

private:
    int server_fd_;
    ConnectionManager connectionManager_;
    void setup_gpio() {setup();}
    void cleanup_gpio() {cleanup();}

    bool setupSocket() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == 0) {
            perror("socket failed");
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            return false;
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("192.168.2.48");
        address.sin_port = htons(PORT);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            return false;
        }

        if (listen(server_fd_, MAXCLIENTS) < 0) {
            perror("listen");
            return false;
        }

        return true;
    }
};

int main() {
    GPIOServer server;
    if (server.start()) {
        server.run();
    }
    return 0;
}