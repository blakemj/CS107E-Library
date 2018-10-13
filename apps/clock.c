#include "../gpio.h"
#include "../timer.h"

void main(void)
{
    char digits[16];
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
  //  gpio_set_output(GPIO_PIN10);
//    gpio_write(GPIO_PIN10, 1);
    int find = 1;
    int firstPin = GPIO_PIN20;
    int firstDigit = GPIO_PIN10;
    char pattern;
    int sec = 0;
    int tenSec = 0;
    int min = 0;
    int tenMin = 0;
    int start = timer_get_ticks();
    while (1) {
        for (int digit = 0; digit < 4; digit++) {
            gpio_set_output(firstDigit + digit);
            if (timer_get_ticks() - start >= 1000000) {
                sec++;
                start = timer_get_ticks();
            }
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
            if (digit == 3) {
                pattern = digits[sec];
            } else if (digit == 2) {
                pattern = digits[tenSec];
            } else if (digit == 1) {
                pattern = digits[min];
            } else {
                pattern = digits[tenMin];
            }
            gpio_write(firstDigit + digit, 1);
            for (int i = 0; i < 8; i++) {
                find = find << i;
                gpio_set_output(firstPin + i); 
                gpio_write(firstPin + i, (find & pattern) >> i);
                find = find >> i;
            }
            timer_delay_us(2500);
            gpio_write(firstDigit + digit, 0);
        }    
    }
//    while(1) {
  //      for (int d = 0; d < 15; d++) {
    //        char pattern = digits[d];
      //      for (int a = 0; a < 0x50000; a++) {
        //        gpio_set_output(GPIO_PIN10);
          //      gpio_write(GPIO_PIN10, 1);
            //    for (int i = 0; i < 8; i++) {
              //      find = find << i;
                //    gpio_set_output(firstPin + i);
                  //  gpio_write(firstPin + i, (find & pattern) >> i);
                    //find = find >> i;
//                }
  //          }
    //    }
//    }
}
