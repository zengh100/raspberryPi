//https://github.com/Milliways2/pi-gpio/blob/main/examples/setGPIO.c
//build: g++ -o pinCtrl pinctrl.cpp -lpi-gpio
#include <stdio.h>
#include <string.h>
//#include <time.h>
#include <chrono>
#include <thread>

#include <pi-gpio.h>


#define	LED	25

int main() {
    setup();

    printf("Set GPIO%d OUTPUT\n", LED);

    setup_gpio(LED, OUTPUT, 0); // Set GPIO 26 OUTPUT

    int i = 0;
    for (;;)
    {
        i++;
        printf("%d On\n", i);
        output_gpio(LED, 1);        // Set LED HIGH
        //delay(500);
	//nanosleep(5000000);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
        printf("%d Off\n", i);
        output_gpio(LED, 0);        // Set LED LOW
        //delayMicroseconds(500000);
        //nanosleep(5000000);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (i == 40) break;
    }

    cleanup();

    return 0;
}
