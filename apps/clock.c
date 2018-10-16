#include "../gpio.h"
#include "../timer.h"

//These constants initialize and define the array of digits, and the first LEDs for the
//digits and digit pieces in the sequence as well as the pins used for the buttons
static char digits[16];
static int FIRST_LED_GPIO = GPIO_PIN20;
static int FIRST_DIGIT_GPIO = GPIO_PIN10;
static int BUTTON_FOR_MINUTES = GPIO_PIN2;
static int BUTTON_FOR_HOURS = GPIO_PIN3;

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
* pattern. The third digit is set to display the ones from the minutes, the second
* is set to display the tens from the minutes, the first is set to display the ones
* from the hours, and the zero pin is set to display the tens from the hours. 
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
        pattern = digits[hr] | 0b10000000; //Unsures that the dot point is always on
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
* in the ones place for minutes before the tens place for minutes needs to be changed), it
* will increase the next time digit by one and will reset back to zero. Further, after one
* minute, the start of the time for one minute is reset to the current raspberry pi's timer
* and the minutes ones digit is increased by one. This repeats every minute exactly. This 
* also allows for two buttons to be pressed--one to set the minutes and one for the hours.
*/ 
void main(void)
{
    gpio_init();
    timer_init();
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

            //Increase the minute by one after one minute has elasped
            if (timer_get_ticks() - start >= 1000000 * 60) {
                min++;
                start = timer_get_ticks();
            }

            //Used to roll over the minutes to the next digit after the final number 
            //for the digit is used--allows for time to increase after 10 minutes into
            //the tens digit, after 60 min into the hours digit, and for the hours digits
            //to reach no further than 12 before they roll back over to 1
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

            //Find the pattern for the given time for the given digit
            //Throughout the loop, this displays all four numbers for the time
            char pattern = figure_out_pattern(digit, min, tenMin, hr, tenHr);
            turn_on_light(digit, pattern);

            //Initialize the GPIO pins to read the inputs from the buttons
            gpio_set_input(BUTTON_FOR_MINUTES);
            gpio_set_input(BUTTON_FOR_HOURS);
            int minButton = gpio_read(BUTTON_FOR_MINUTES);
            int hrButton = gpio_read(BUTTON_FOR_HOURS);
 
            //The buttons are wired to always give 3.3V (aka 1) to the pins until they
            //are pressed when the pins become wired to ground (0V aka 0)
            //Gives the user a fifth of a second to depress the button before increasing
            //again
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
