#include "gpio.h"
#include "gpioextra.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "mouse.h"
#include "timer.h"
#include "assert.h"
#include "printf.h"

//These define the mouse gpio pins
#define MOUSE_CLK GPIO_PIN23
#define MOUSE_DATA GPIO_PIN24

//This defines the number of data bits being read/written
#define NUM_DATA_BITS 8

//These define commands to write to the mouse
#define CMD_RESET 0xFF
#define CMD_ENABLE_DATA_REPORTING 0xF4

//This initializes the ringbuffer
static rb_t *rb;

//These allow for functions to be called
static void mouse_write(unsigned char data);
int mouse_read_scancode(void);
static void mouse_handler(unsigned int pc);

/*
* This spins until the clock has a falling edge
*/
void wait_for_falling_clock_edge() {
    while (gpio_read(MOUSE_CLK) == 0) {}
    while (gpio_read(MOUSE_CLK) == 1) {}
}

bool mouse_init(void)
{
  rb = rb_new();
  //This initializes the gpio pins
  gpio_set_function(MOUSE_CLK, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_CLK);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);
  //This initializes the interrupts
  gpio_enable_event_detection(MOUSE_CLK, GPIO_DETECT_FALLING_EDGE);
  bool ok = interrupts_attach_handler(mouse_handler);
  assert(ok);
  interrupts_enable_source(INTERRUPTS_GPIO3);
  interrupts_global_enable();
  //This sets up the mouse
  timer_delay_us(50);
  mouse_write(CMD_RESET);
  unsigned int test = mouse_read_scancode();
  if (test != 0xaa) return false;
  unsigned int id = mouse_read_scancode();
  if (id != 0x00) return false;
  mouse_write(CMD_ENABLE_DATA_REPORTING);
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
      evt.dx = (-1) * (256 - scancodeTwo);
  } else {
      evt.dx = scancodeTwo;
  }
  if (scancodeOne & (1 << 5)) {
      evt.dy = (-1) * (256 - scancodeThree);
  } else {
      evt.dy = scancodeThree;
  }
  return evt;
}

int mouse_read_scancode(void)
{
    while (1) {
        int charCode = 0;
        if (rb_dequeue(rb, &charCode)) return (unsigned char) charCode;  
    }
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
      gpio_write(MOUSE_DATA, (bit >> i));
      if (bit) numOne++;
      wait_for_falling_clock_edge();
  }
  gpio_write(MOUSE_DATA, (numOne + 1) % 2);
  wait_for_falling_clock_edge();
  gpio_write(MOUSE_DATA, 1);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);
  while(gpio_read(MOUSE_DATA) == 1) {}
  while(gpio_read(MOUSE_CLK) == 1) {}
  interrupts_enable_source(INTERRUPTS_GPIO3);
}

static int correct_bit = 0;
static int bit_num = 0;
static int scancode = 0;
static int numOnes = 0;
static void mouse_handler(unsigned int pc)
{
    if (gpio_check_and_clear_event(MOUSE_CLK)) {
        int bit = gpio_read(MOUSE_DATA);
        if (!bit_num && !bit) {
            correct_bit = 1;
            bit_num++;
        } else if (correct_bit && bit_num == NUM_DATA_BITS + 1) {
            if (bit) numOnes++;
            if (numOnes % 2 != 1) correct_bit = 0;
            bit_num++;
        } else if (correct_bit && bit_num == NUM_DATA_BITS + 2) {
            if (bit) rb_enqueue(rb, scancode);
            correct_bit = 0;
            scancode = 0;
            bit_num = 0;
            numOnes = 0;
        } else if (correct_bit) {
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
}
