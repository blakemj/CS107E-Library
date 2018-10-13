#include "timer.h"

void timer_init(void) {
}

/*
* This function will return the value inside the timer address for the 
* raspberry pi. 
*/
unsigned int timer_get_ticks(void) {
    int* timerAddress = (int*)0x20003004;
    return *timerAddress;
}

/*
* This function will return the delay in microseconds. The start variable
* starts at the timer inside the raspberry pi. The variable is set to volatile
* so that the compiler consistently checks the loop.
*/
void timer_delay_us(unsigned int usecs) {
    volatile unsigned int start = timer_get_ticks();
    while (timer_get_ticks() - start < usecs) { /* spin */ }
}

//This function will return the delay in miliseconds.
void timer_delay_ms(unsigned int msecs) {
    timer_delay_us(1000*msecs);
}

//This function will return the delay in seconds
void timer_delay(unsigned int secs) {
    timer_delay_us(1000000*secs);
}
