#include "timer.h"

void timer_init(void) {
}

unsigned int timer_get_ticks(void) {
    int* timerAddress = (int*)0x20003004;
    return *timerAddress;  // TODO: Your code goes here.
}

void timer_delay_us(unsigned int usecs) {
    volatile unsigned int start = timer_get_ticks();
    while (timer_get_ticks() - start < usecs) { /* spin */ }
}

void timer_delay_ms(unsigned int msecs) {
    timer_delay_us(1000*msecs);
}

void timer_delay(unsigned int secs) {
    timer_delay_us(1000000*secs);
}
