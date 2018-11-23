#include "gpio.h"
#include "gpioextra.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "mouse.h"
#include "timer.h"
#include "assert.h"
#include "printf.h"

#define MOUSE_CLK GPIO_PIN23
#define MOUSE_DATA GPIO_PIN24

#define NUM_DATA_BITS 8

#define CMD_RESET 0xFF
#define CMD_ENABLE_DATA_REPORTING 0xF4

static rb_t *rb;

static void mouse_write(unsigned char data);
int mouse_read_scancode(void);
static void mouse_handler(unsigned int pc);

void wait_for_falling_clock_edge() {
    while (gpio_read(MOUSE_CLK) == 0) {}
    while (gpio_read(MOUSE_CLK) == 1) {}
}

bool mouse_init(void)
{
  rb = rb_new();

  gpio_set_function(MOUSE_CLK, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_CLK);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);

    gpio_enable_event_detection(MOUSE_CLK, GPIO_DETECT_FALLING_EDGE);
    bool ok = interrupts_attach_handler(mouse_handler);
    assert(ok);
    interrupts_enable_source(INTERRUPTS_GPIO3);
    interrupts_global_enable();

  mouse_write(CMD_RESET);
  timer_delay_us(500);
//  int ack1 = mouse_read_scancode();
//  printf("\n%x\n", ack1);
//  int bat = mouse_read_scancode();
//  int id = mouse_read_scancode();
//  printf("\n%x, %x, %x\n", ack1, bat, id);
  mouse_write(CMD_ENABLE_DATA_REPORTING);
//  int ack2 = mouse_read_scancode();
  // FIXME: Initialize mouse.
//  printf("\n%x, %x, %x, %x\n", ack1, bat, id, ack2);
  timer_delay_us(500);
  return true;
}

mouse_event_t mouse_read_event(void)
{
  mouse_event_t evt;
  int scancodeOne = mouse_read_scancode();
  evt.left = scancodeOne & 1;
  evt.right = scancodeOne & (1 << 1);
  evt.middle = scancodeOne & (1 << 2);
  evt.x_overflow = scancodeOne & (1 << 6);
  evt.y_overflow = scancodeOne & (1 << 7);
  int scancodeTwo = mouse_read_scancode();
  int scancodeThree = mouse_read_scancode();
  if (scancodeOne & (1 << 4)) {
      evt.dx = (-1) * scancodeTwo;
  } else {
      evt.dx = scancodeTwo;
  }
  if (scancodeOne & (1 << 5)) {
      evt.dy = (-1) * scancodeThree;
  } else {
      evt.dy = scancodeThree;
  }
  // FIXME: Read scancode(s) and fill in evt.

  return evt;
}

int mouse_read_scancode(void)
{
    while (1) {
        int charCode = 0;
        if (rb_dequeue(rb, &charCode)) return (unsigned char) charCode;  
    }
/*
  // FIXME: Read from ring buffer.
    wait_for_falling_clock_edge();
    if (gpio_read(MOUSE_DATA) == 0) {
        int binaryDigit = 1;
        int numOnes = 0;
        unsigned int totalChar = 0; 
        //Read 8 Data bits
        for (int i = NUM_DATA_BITS - 1; i >=0; i--) {
            wait_for_falling_clock_edge();
            totalChar = totalChar + binaryDigit * gpio_read(MOUSE_DATA);
            if (gpio_read(MOUSE_DATA) == 1) numOnes++;
            binaryDigit = binaryDigit * 2;
        }
        //Check Parity Bit
        wait_for_falling_clock_edge();
        if (gpio_read(MOUSE_DATA) == 1) numOnes++;
        if (numOnes % 2 != 1) totalChar = mouse_read_scancode();
        //Check Stop Bit
        wait_for_falling_clock_edge();
        if(gpio_read(MOUSE_DATA) == 0) totalChar =  mouse_read_scancode();
        return totalChar;
    } else {
        return mouse_read_scancode();
    }
    return 0;
*/
}

static void mouse_write(unsigned char data)
{
  interrupts_disable_source(INTERRUPTS_GPIO3);
  gpio_set_function(MOUSE_CLK, GPIO_FUNC_OUTPUT);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_OUTPUT);
  gpio_write(MOUSE_CLK, 0);
  timer_delay_us(100);
  gpio_write(MOUSE_DATA, 0);
  gpio_set_function(MOUSE_CLK, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_CLK);
  while (gpio_read(MOUSE_CLK) == 1) {}
  int numOne = 0;
  for (int i = 0; i < NUM_DATA_BITS; i++) {
      int bit = data & (1 << i);
//      printf("write: %x\n", (bit >> i));
      gpio_write(MOUSE_DATA, (bit >> i));
      if (bit) numOne++;
      wait_for_falling_clock_edge();
  }
  gpio_write(MOUSE_DATA, (numOne + 1) % 2);
//  printf("write: %x\n", (numOne + 1) % 2);
  wait_for_falling_clock_edge();
  gpio_write(MOUSE_DATA, 1);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);
  while(gpio_read(MOUSE_DATA) == 1) {}
  while(gpio_read(MOUSE_CLK) == 1) {}
  interrupts_enable_source(INTERRUPTS_GPIO3);
  // FIXME: Send host->mouse packet.
}

static int correct_bit = 0;
static int bit_num = 0;
static int scancode = 0;
static int numOnes = 0;
//static int zeroyet = 0;
//static int count = 0;
static void mouse_handler(unsigned int pc)
{
    if (gpio_check_and_clear_event(MOUSE_CLK)) {
        int bit = gpio_read(MOUSE_DATA);
  //      printf("%x", bit);
        if (!bit_num && !bit) {
            correct_bit = 1;
            bit_num++;
        } else if (correct_bit && bit_num == NUM_DATA_BITS + 1) {
            if (bit) numOnes++;
            if (numOnes % 2 != 1) correct_bit = 0;
            bit_num++;
        } else if (correct_bit && bit_num == NUM_DATA_BITS + 2) {
            if (bit) rb_enqueue(rb, scancode);
//            count++;
            correct_bit = 0;
            scancode = 0;
            bit_num = 0;
            numOnes = 0;
        } else if (correct_bit) {
  //          if (count < 2) {
    //            if (!bit && !zeroyet) {
      //              zeroyet = 1;
        //            return;
          //      }
            //    if (!bit && zeroyet) {
              //      zeroyet = 0;
//                }
  //              if (bit && zeroyet) {
    //                zeroyet = 0;
      //              bit_num++;
        //            return;
          //      }
         //   }
//            printf("%x", bit);
            scancode = scancode | (bit << (bit_num - 1));
            if (bit) numOnes++;
            bit_num++;
        } else {
            correct_bit = 0;
            scancode = 0;
            bit_num = 0;
            numOnes = 0;
        }
    }
  // FIXME: Handle event on mouse clock line
}
