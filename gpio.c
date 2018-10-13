#include "gpio.h"

//These constants are the addresses for their first respective address
static unsigned int FSEL_ADDRESS = 0x20200000;
static unsigned int SET_ADDRESS = 0x2020001C;
static unsigned int CLR_ADDRESS = 0x20200028;
static unsigned int LEV_ADDRESS = 0x20200034;

void gpio_init(void) {
}

/*
* This function is used to set a given pin to a given function. This will first 
* check to make sure that the pin and function are within the given ranges allowed
* by the rasperry pi. It will then calculate the starting bit position for the 
* given pin in the register, and the address that the given pin is in. Finally,
* it will set the value of the register so that the pin is set to its given 
* function. This will also make sure that it does not change the function value
* for any of the other pins in the register.
*/
void gpio_set_function(unsigned int pin, unsigned int function) {
    if (pin > GPIO_PIN_LAST || pin < GPIO_PIN_FIRST) return;
    if (function > GPIO_FUNC_ALT3 || function < GPIO_FUNC_INPUT) return;
    int StartBit = (pin % 10) * 3;
    int* FSEL = (int*)FSEL_ADDRESS + (pin / 10);
    *FSEL = ((~(7 << StartBit)) & *FSEL) | (function << StartBit);
}

/*
* This function is used to get the function for a given pin. This will first 
* check to make sure that the pin is within the given ranges allowed
* by the rasperry pi. It will then calculate the starting bit position for the 
* given pin in the register, and the address that the given pin is in. Finally,
* it will get the 3 bit value for the given pin in the register by comparing it
* to 3 1-bits shifted to the pin's position. 
*/
unsigned int gpio_get_function(unsigned int pin) {
    if (pin > GPIO_PIN_LAST || pin < GPIO_PIN_FIRST) return GPIO_INVALID_REQUEST;
    int StartBit = (pin % 10) * 3;
    int* FSEL = (int*)FSEL_ADDRESS + (pin / 10);
    return (*FSEL & (7 << StartBit)) >> StartBit;
}

//This can be called to specifically set a pin to input
void gpio_set_input(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

//This can be called to specifically set a pin to output
void gpio_set_output(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

/*
* This function is used to set a given pin to a given value. First, it will check
* to make sure that the pin is within the pin values for the rasperry pi. Then,
* it will set the value of the SET or CLR pointer depending on the pin (the first
* 31 pins are in the first register, and the rest are in the second register).
* Finally, it will set either the SET or the CLR register depending on if the 
* value is 1 or 0 respectively. It will set the value in the register to 1 for 
* the givin bit for the given register. These registers are set so that there is
* no need for read then write.
*/
void gpio_write(unsigned int pin, unsigned int value) {
    if (pin > GPIO_PIN_LAST || pin < GPIO_PIN_FIRST) return;
    int* SET;
    int* CLR;
    if (pin <= 31) { //This is the last pin in the first address
        SET = (int*)SET_ADDRESS;
        CLR = (int*)CLR_ADDRESS;
    } else {
        pin = pin - 32; //This sets pin 32 to 0 (0th pin in the address)
        SET = (int*)SET_ADDRESS + 1;
        CLR = (int*)CLR_ADDRESS + 1;
    }
    if (value == 1) {
        *SET = (1 << pin);
    } else if (value == 0) {
        *CLR = (1 << pin);
    }
}

/*
* This function will read the value associated with a given pin. When the set or
* clr register is set for a pin, the LEV register is set for the given value. 
* First, this function will ensure that the pin is within the values allowed for
* the pins on the raspberry pi. It will then set the LEV pointer equal to the 
* register address for the pin's given register. It will then find the bit in 
* the register for the given pin and return it.
*/
unsigned int gpio_read(unsigned int pin) {
    if (pin > GPIO_PIN_LAST || pin < GPIO_PIN_FIRST) return GPIO_INVALID_REQUEST;
    int* LEV;
    if (pin <= 31) { //This is the last pin in the first address
        LEV = (int*)LEV_ADDRESS;
    } else {
        pin = pin - 32; //This sets pin 32 to 0 (0th pin in the address)
        LEV = (int*)LEV_ADDRESS + 1;
    }
    return (*LEV & (1 << pin)) >> pin;
}
