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

//Build: g++ -o server6 tcp_server_v6_addSetCmd.cpp -lpi-gpio
//Build: g++ -o client4 tcp_client_v4.cpp -lpi-gpio
using namespace std;

constexpr int PORT = 8081;
constexpr int BUFFER_SIZE = 1024;

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
        int f = gpio_function(pin);
        int p = (f == 0) ? get_pullupdn(pin) : 0; // only input
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

void cl_new_socket(int server_fd, int  new_socket, int threadID) 
{
    std::string welcome = "Welcome! type example: get8";
    char buffer[BUFFER_SIZE] = {0};
    send(new_socket, welcome.c_str(), welcome.size(), 0);
    ssize_t valread = 0;
    while(1)
    {
    	// Read and echo the received message
        buffer[0] = '\0'; //reset buffer to be empty
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

	    //std::string bufString = buffer;
        //check if "quit" is received
        if(std::string(buffer).compare("quit") == 0) 
        {
            std::cout << "Client quit. Exiting this conversation.\n";
            break;
        }
 	    std::string bufString = std::string(buffer);
        bool  gpioCmd = false;
        if(bufString.substr(0,3).compare("get") == 0)
	    {
            gpioCmd = process_get(new_socket, bufString);
            if(!gpioCmd)
            {
                std::string prompt = "Not a valid query! type example: get8";
                send(new_socket, prompt.c_str(), prompt.size(), 0);
                //send(new_socket, buffer, valread, 0);
                std::cout << "send: " << prompt << std::endl;
            }            
        }
        if(bufString.substr(0,3).compare("set") == 0)
        {
            // expect to recieve: setxxyz where xx=pin number, y=input(0) or output(1), z=high(1) or low(0)
            // new design: set,pin,input/output,value
            gpioCmd = process_set(new_socket, bufString);
            if(!gpioCmd)
            {
                std::string prompt = "Not a valid request! type example: set,8,1,1";
                send(new_socket, prompt.c_str(), prompt.size(), 0);
                //send(new_socket, buffer, valread, 0);
                std::cout << "send: " << prompt << std::endl;
            }            
        }
    };
}

int main() {
    int server_fd, new_socket;
    std::vector<int> clientSockets;
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
    // set up gpio
    setup();

    // run accept in a loop so that multiple clients can be accepted
    const int MAXCLIENTS = 3;
 	std::thread socket_threads[MAXCLIENTS];
	int i = 0;
	while (i < MAXCLIENTS)
    {
        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "connected to IP:" << inet_ntoa(address.sin_addr) << ", port:" << ntohs(address.sin_port) << std::endl;
            //create a new thread for this client
            clientSockets.push_back(new_socket);
			socket_threads[i] = thread(cl_new_socket, server_fd, new_socket, i);
			cout << "While looop i=" << i << endl;
        }
        i++;
    }
    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }



    cleanup();
    // Close the socket
    close(new_socket);
    close(server_fd);
    return 0;
}
/*
Code refactoring is the process of restructuring existing code without changing its external behavior. 
The goal is to improve the code's design, structure, and readability, while also making it easier to maintain. 
*/
/*
todo list 2025.03.04 for next week
- manage clients
    - remove quit clients and allow new client to connect
    - broadcast set command to all clients
*/