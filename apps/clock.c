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
static char figure_out_pattern(int digit, int min, int tenMin, int hr, int tenHr) {
    char pattern;
    if (digit == 3) {
        pattern = digits[min];
    } else if (digit == 2) {
        pattern = digits[tenMin];
    } else if (digit == 1) {
        pattern = digits[hr] | 0b10000000;
    } else {
        pattern = digits[tenHr];
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
    int min = 0;
    int tenMin = 0;
    int hr = 0;
    int tenHr = 0;
    int start = timer_get_ticks();
    int buttonPress = timer_get_ticks();
    while (1) {
        for (int digit = 0; digit < 4; digit++) {
            gpio_set_output(FIRST_DIGIT_GPIO + digit);
            if (timer_get_ticks() - start >= 1000000 * 60) {
                min++;
                start = timer_get_ticks();
            }
            if (min == 10) {
                tenMin++;
                min = 0;
            }
            if (tenMin == 6) {
                hr++;
                tenMin = 0;
            }
            if (hr == 10) {
                tenHr++;
                hr = 0;
            }
            if (tenHr == 1 && hr == 3) {
                tenHr =0;
                hr = 1;
            }
            char pattern = figure_out_pattern(digit, min, tenMin, hr, tenHr);
            turn_on_light(digit, pattern);
            gpio_set_input(GPIO_PIN2);
            gpio_set_input(GPIO_PIN3);
            int minButton = gpio_read(GPIO_PIN2);
            int hrButton = gpio_read(GPIO_PIN3);
            if (minButton == 0 && timer_get_ticks() - buttonPress >= 200000) {
                min++;
                buttonPress = timer_get_ticks();
            }
            if (hrButton == 0 && timer_get_ticks() - buttonPress >= 200000) {
                hr++;
                buttonPress = timer_get_ticks();
            }
        }    
    }
}
