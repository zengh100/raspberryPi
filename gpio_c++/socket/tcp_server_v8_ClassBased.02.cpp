#include <iostream>
#include <string>
#include <sstream>
#include <vector>
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
#include <map>

using namespace std;

constexpr int PORT = 8081;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAXCLIENTS = 3;

std::vector<std::string> split(std::string my_str)
{
   //string my_str = "ABC,XYZ,Hello,World,25,C++";
   vector<string> result;
   stringstream s_stream(my_str); //create string stream from the string
   while(s_stream.good()) {
      string substr;
      getline(s_stream, substr, ','); //get first string delimited by comma
      result.push_back(substr);
   }
   for(int i = 0; i<result.size(); i++) {    //print all splitted strings
      cout << result.at(i) << endl;
   }
   return result;
}
bool convertStrToInteger(std::string strIn, int& i)
{
    // retrive pin, input/output, value
    i = atoi(strIn.c_str());
    //itoa (pin,buffer,10);
    std::stringstream ss;
    ss << i;
    std::string iStr2 = ss.str();
    //std::string pinStr2 = std::string(buffer);
    std::cout << "strIn=" << strIn << ", iStr2=" << iStr2 << "\n";
    if(strIn.compare(iStr2) != 0) //they are different, conversion failed 
    {
        std::cout << "invalid number, reset integer to -1\n";
        i = -1;
        return false;
    }
    return true;
}

bool checkRange(int v, int vMin, int vMax)
{
    return vMin <= v && v <= vMax;
}

// Utility functions (like process_get, process_set, etc.) go here, unchanged.
bool process_get(const int& new_socket, std::string bufString)
{
    bool gpioCmd = false;
    int valread = bufString.size();
    //example string: "get8", "get19"
    std::string  pinStr = bufString.substr(3,valread); //"get8" should return "8"
    std::cout << "pinStr=" << pinStr << "\n";
    //what if pinStr == "t1"?
    int pin = atoi(pinStr.c_str());
    //itoa (pin,buffer,10);
    std::stringstream ss;
    ss << pin;
    std::string pinStr2 = ss.str();
    //std::string pinStr2 = std::string(buffer);
    std::cout << "pinStr2=" << pinStr2 << "\n";
    if(pinStr.compare(pinStr2) != 0) {
        std::cout << "invalid number, reset pin to -1\n";
        pin =-1;
    }
    std::cout << "pin=" << pin << "\n";
    //check pin range
    if (pin >=0 && pin < 28)
    {
        gpioCmd = true;
        int n = input_gpio(pin);
        std::cout << "n=" << n << "\n";
        int f = gpio_function(pin);
        std::cout << "f=" << f << "\n";
        int p = (f == 0) ? get_pullupdn(pin) : 0; // only input
        std::cout << "p=" << p << "\n";
        std::stringstream ss;
        if (f == 0)
        {
            //printf("GPIO %2i Value=%i Function %i Pull %i\n", pin, n, f, p);
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f << " Pull=" << p;
        }
        else
        {
            //printf("GPIO %2i Value=%i Function %i\n", pin, n, f);
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f;
        }
        std::string str = ss.str();
        std::cout << "sending\n";
        send(new_socket, str.c_str(), str.size(), 0);
        std::cout << "sent: " << str << std::endl;
    }
    return gpioCmd;
}

bool process_set(const int& new_socket, std::string bufString)
{
    cout << "process_set: bufString=" << bufString << "\n";
    bool gpioCmd = false;
    int valread = bufString.size();
    //example string: "set,8,1,1", "set,19,0,0"

    std::vector<std::string> params = split(bufString);
    if(params.size() < 4)
    {
        cout << "it should have 4 strings in params\n";
        return false;
    }
    //check first string is "set"
    if(params[0] != "set") 
    {
        cout << "process_set: params[0]=" << params[0] << " return false\n";
        return false;
    }

    // retrive pin, input/output, value
    int pin = -1;
    int mode = -1; //1=ouput, 0 = input
    int value = -1; //1=HIGH, 0 = LOW
    if(!convertStrToInteger(params[1], pin)) return false;
    if(!convertStrToInteger(params[2], mode)) return false;
    if(!convertStrToInteger(params[3], value)) return false;

    cout << "pin:" << pin << ", mode:" << mode << ", value:" << value << "\n";
    // check pin is between [0,27]
    // check mode is between [0,1]
    // check value is between [0,1]
    if(!checkRange(pin, 0,27)) return false;
    if(!checkRange(mode, 0,1)) return false;
    if(!checkRange(value, 0,1)) return false;
    //2025.02.18: will continue from here next class
    //check pin range
    if (pin >=0 && pin < 28)
    {
        gpioCmd = true;

        if(mode==1)
        {
            setup_gpio(pin, OUTPUT, value); // Set pin OUTPUT and value to be value
            output_gpio(pin, value);
        }
        else
            setup_gpio(pin, INPUT, 0); // Set pin INTPUT
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Read the same pin and send back the readings
        int n = input_gpio(pin);
        int f = gpio_function(pin);
        int p = (f == 0) ? get_pullupdn(pin) : 0; // only input (f==0?) p: pulldown?
        std::stringstream ss;
        if (f == 0)
        {
            //printf("GPIO %2i Value=%i Function %i Pull %i\n", pin, n, f, p);
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f << " Pull=" << p;
        }
        else
        {
            //printf("GPIO %2i Value=%i Function %i\n", pin, n, f);
            ss << "GPIO " << pin << ": Value=" << n << " Function=" << f;
        }

        std::string str = ss.str();
        send(new_socket, str.c_str(), str.size(), 0);
        std::cout << "sent: " << str << std::endl;
    }

    return gpioCmd;
}

// Connection class to handle individual client connections
class Connection {
public:
    Connection(int socket, sockaddr_in addr, int* numOfClients) 
        : socket_(socket), address_(addr), m_numOfClients(numOfClients) {}
    
    void handle() {
        std::string welcome = "Welcome! type example: get8";
        send(socket_, welcome.c_str(), welcome.size(), 0);
        
        char buffer[BUFFER_SIZE] = {0};
        ssize_t valread;
        
        while (true) {
            buffer[0] = '\0';
            valread = 0;
            size_t elapsed_ms = 0;
            size_t timeout_ms = 60000;  // 1 minute timeout
            const size_t sleep_ms = 100;

            // Wait for the client message or timeout
            while (valread <= 0 && elapsed_ms < timeout_ms) {
                valread = read(socket_, buffer, BUFFER_SIZE);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                elapsed_ms += sleep_ms;
            }

            if (valread <= 0) {
                std::cout << "Timeout waiting for client message. Disconnecting...\n";
                break;
            }

            buffer[valread] = '\0';
            std::string bufString = std::string(buffer);
            std::cout << "Received: " << bufString << std::endl;

            // Process "quit" message
            if (bufString == "quit") {
                *m_numOfClients -= 1;
                break;
            }

            // Process "get" or "set" commands
            bool gpioCmd = false;
            if (bufString.substr(0, 3) == "get") {
                gpioCmd = process_get(socket_, bufString);
            } else if (bufString.substr(0, 3) == "set") {
                gpioCmd = process_set(socket_, bufString);
            }

            // Send feedback if the command was invalid
            if (!gpioCmd) {
                std::string prompt = "Not a valid request! type example: set,8,1,1";
                send(socket_, prompt.c_str(), prompt.size(), 0);
            }
        }
        // This thread is to be finished
        std::cout << "This thread is to be ended" << std::endl;
    }

    string getClientInfo() const {
        return "IP: " + string(inet_ntoa(address_.sin_addr)) + ", Port: " + to_string(ntohs(address_.sin_port));
    }

    int getSocket() const { return socket_; }

private:
    int socket_;
    sockaddr_in address_;
    int* m_numOfClients;
};

// ConnectionManager class to handle multiple connections
class ConnectionManager {
public:
    void addConnection(int socket, sockaddr_in addr, int* numOfClients) {
        auto connection = std::make_shared<Connection>(socket, addr);
        //connections_.push_back(t);
        auto t = std::thread(&Connection::handle, connection);
        //threads.push_back(t);
        //t.join();
        connectionMap[connection] = t;
    }

    void printActiveConnections() const {
        // for (const auto& conn : connections_) {
        //     std::cout << conn->getClientInfo() << std::endl;
        // }
        for (const auto& conn : connections_) {
            std::cout << conn->getClientInfo() << std::endl;
        }
        for (const auto& [conn, t] : connectionMap)
            std::cout << conn->getClientInfo() << ", thread=" <<  << std::endl;       
    }

    void removeConnection(shared_ptr<Connection> connection) 
    {
        if(connectionMap.find(connection))
        {
            connectionMap.erase(connection);
        }
    }

    int getNumOfConnections() {return connectionMap.size();}
private:
    //vector<std::thread> threads;
    //vector<std::shared_ptr<Connection>> connections_;
    std::map<std::shared_ptr<Connection>, std::thread> connectionMap;
};

// Server class to manage server setup and incoming connections
class Server {
public:
    Server() : server_fd(-1) {
        setupServer();
    }

    ~Server() {
        if (server_fd != -1) {
            close(server_fd);
        }
    }

    void run() {
        while (true) {
            //numOfClients = connectionManager.getNumOfConnections
            if (numOfClients < MAXCLIENTS) {
                sockaddr_in clientAddr;
                socklen_t addrlen = sizeof(clientAddr);

                int new_socket = accept(server_fd, (struct sockaddr*)&clientAddr, &addrlen);
                if (new_socket < 0) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                std::cout << "New connection from IP: " << inet_ntoa(clientAddr.sin_addr)
                          << ", Port: " << ntohs(clientAddr.sin_port) << std::endl;

                connectionManager.addConnection(new_socket, clientAddr, &numOfClients);
                numOfClients++;
                std::cout << "Number of clients: " << numOfClients << std::endl;
            }

            //connectionManager.handleConnections();
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    }

private:
    void setupServer() {
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("192.168.2.48");
        address.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, MAXCLIENTS) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << PORT << std::endl;
    }

    int server_fd;
    int numOfClients = 0;
    ConnectionManager connectionManager;
};

int main() {
    // set up gpio
    setup(); //gpio setup function
    Server server;
    server.run();
    cleanup(); //gpio cleanup function
    return 0;
}
