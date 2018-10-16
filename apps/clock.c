#include "../gpio.h"
#include "../timer.h"

//These constants initialize and define the array of digits, and the first LEDs for the
//digits and digit pieces in the sequence
static char digits[16];
static int FIRST_LED_GPIO = GPIO_PIN20;
static int FIRST_DIGIT_GPIO = GPIO_PIN10;

/*
* This function simply initializes the array of possible digit types 0-F. The LED parts
* are in order in the binary from A on the far right to DP on the far left.
*/
static void digits_init(void) {
    digits[0] = 0b00111111;
    digits[1] = 0b00000110;
    digits[2] = 0b01011011;
    digits[3] = 0b01001111;
    digits[4] = 0b01100110;
    digits[5] = 0b01101101;
    digits[6] = 0b01111101;
    digits[7] = 0b00000111;
    digits[8] = 0b01111111;
    digits[9] = 0b01100111;
    digits[10] = 0b01110111;
    digits[11] = 0b01111100;
    digits[12] = 0b00111001;
    digits[13] = 0b01011110;
    digits[14] = 0b01111001;
    digits[15] = 0b01110001;
}

/*
* This function is used to turn on the light. The GPIO pins for the LEDs are set to be
* on in the specific pattern passed in for the digit. Only the digit pin for the current
* digit is turned on on the function, and after a delay for an unnoticable amount of time,
* the digit pin is turned off in preparation for the next. By doing so, we can flash all
* the digits on without having a pin for each small LED on the clock.
*/
static void turn_on_light(int digit, char pattern) {
    int find = 1;
    gpio_write(FIRST_DIGIT_GPIO + digit, 1);
    for (int i = 0; i < 8; i++) {
        find = find << i;
        gpio_set_output(FIRST_LED_GPIO + i);
        gpio_write(FIRST_LED_GPIO + i, (find & pattern) >> i);
        find = find >> i; 
    }
    timer_delay_us(2500);
    gpio_write(FIRST_DIGIT_GPIO + digit, 0);
}

/*
* This function will simply set the digits from right to left to display the correct
* pattern. The third digit is set to display the ones from the seconds, the second
* is set to display the tens from the seconds, the first is set to display the ones
* from the minutes, and the zero pin is set to display the tens from the minutes. 
* Depending on what number of given time a digit is on, it will find the pattern from
* the pattern array that corresponds and return it.
*/
static char figure_out_pattern(int digit, int sec, int tenSec, int min, int tenMin) {
    char pattern;
    if (digit == 3) {
        pattern = digits[sec];
    } else if (digit == 2) {
        pattern = digits[tenSec];
    } else if (digit == 1) {
        pattern = digits[min];
    } else {
        pattern = digits[tenMin];
    }
    return pattern;
}

/*
* This function is used to run the clock. First, it initializes the variables being used.
* It then initializes the start time of the program (so that the program starts at zero).
* Then, in an endless loop, it will switch between the four digits and display the pattern
* for the time on that digit. Once a given time number has reached its max (such as 9 sec
* in the ones place for seconds before the tens place for seconds needs to be changed), it
* will increase the next time digit by one and will reset back to zero. Further, after one
* second, the start of the time for one second is reset to the current raspberry pi's timer
* and the seconds ones digit is increased by one. This repeats every second exactly.
*/ 
void main(void)
{
    digits_init();
    int sec = 0;
    int tenSec = 0;
    int min = 0;
    int tenMin = 0;
    int start = timer_get_ticks();
    while (1) {
        for (int digit = 0; digit < 4; digit++) {
            gpio_set_output(FIRST_DIGIT_GPIO + digit);
           
            //Increase the second by one after one second has elasped
            if (timer_get_ticks() - start >= 1000000) {
                sec++;
                start = timer_get_ticks();
            }
           

            //Used to roll over the second to the next digit after the final number 
            //for the digit is used--allows for time to increase after 10 seonds into
            //the tens digit, after 60 min into the minutes digit, and after 10 minutes
            //into the tens digit
            if (sec == 10) {
                tenSec++;
                sec = 0;
            }
            if (tenSec == 6) {
                min++;
                tenSec = 0;
            }
            if (min == 10) {
                tenMin++;
                min = 0;
            }
   
            
            //Find the pattern for the given time for the given digit
            //Throughout the loop, this displays all four numbers for the time
            char pattern = figure_out_pattern(digit, sec, tenSec, min, tenMin);
            turn_on_light(digit, pattern);
        }    
    }
}
