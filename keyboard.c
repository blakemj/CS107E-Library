#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"

const unsigned int CLK  = GPIO_PIN23;
const unsigned int DATA = GPIO_PIN24; 

void wait_for_falling_clock_edge() {
    while (gpio_read(CLK) == 0) {}
    while (gpio_read(CLK) == 1) {}
}

void keyboard_init(void) 
{
    gpio_set_input(CLK); 
    gpio_set_pullup(CLK); 
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA); 
}

unsigned char keyboard_read_scancode(void) 
{
    // TODO: Your code here
    return 0x55;
}

int keyboard_read_sequence(unsigned char seq[])
{
    // TODO: Your code here
    return 0;
}

key_event_t keyboard_read_event(void) 
{
    // TODO: Your code here
    key_event_t event;
    return event;
}


unsigned char keyboard_read_next(void) 
{
    // TODO: Your code here
    return '!';
}
