#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include "httplib.h"
//#include <pi-gpio.h>  // Assuming you have the GPIO library
#include <arpa/inet.h>

// Constants
constexpr int MAX_PIN = 27;

// Function to get the status of all GPIO pins
std::string get_pin_status() {
    std::stringstream status_stream;
    for (int pin = 0; pin <= MAX_PIN; ++pin) {
        // int value = input_gpio(pin);  // Get the current value of the pin (assuming input)
        // int function = gpio_function(pin);  // Get the pin function
        // int pull = (function == 0) ? get_pullupdn(pin) : 0;  // Only for input pins
        int value = 0;
        int function = 0;
        int pull = 0;
        status_stream << "GPIO " << pin << ": Value=" << value << " Function=" << function << " Pull=" << pull << "<br>";
    }
    std::cout << "Status: " << status_stream.str() << std::endl;
    return status_stream.str();
}

// Web server logic to serve pin status
void start_web_server() {
    httplib::Server svr;
    std::cout << "Started Web Server at " << time(nullptr) << std::endl;

    // Define the HTTP endpoint for displaying the pin status
    svr.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        // Generate the HTML content to display pin statuses
        std::string status = get_pin_status();
        std::string html = "<html><body><h1>GPIO Pin Status</h1>" + status + "<br><p>Page refreshes every minute.</p></body></html>";
        
        // Set the response content type and send the HTML
        std::cout << "Set content at " << time(nullptr) << std::endl;
        res.set_content(html, "text/html");
    });

    // Start the web server on localhost, port 8080. so only localhost:8080 works
    //svr.listen("localhost", 8080);
    //change port
    //svr.listen("localhost", 8180);

    //Modify the binding: change it to 0.0.0.0 or * to make your application listen on all available network interfaces.
    // So that http://<ip-address>:8180 works too.
    svr.listen("0.0.0.0", 8180);
}

// Main function to setup the server and the GPIO
int main() {
    // Initialize the GPIO pins
    // setup();
    // std::cout << "GPIO pins initialized at " << time(nullptr) << std::endl;

    std::string status = get_pin_status();
    std::cout << "get_pin_status at " << time(nullptr) << std::endl;
    std::cout << "status: " << status << std::endl;

    // Run the web server in a separate thread
    std::thread web_server_thread(start_web_server);

    // Periodic update every minute (for server-side status fetch)
    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        // Fetch and update the pin statuses every minute
        std::cout << "Pin statuses updated at " << time(nullptr) << std::endl;
    }

    // Join the web server thread (though it's running indefinitely)
    web_server_thread.join();

    //cleanup();  // Cleanup GPIO
    return 0;
}
