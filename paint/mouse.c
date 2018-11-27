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

/*
* This function initializes the mouse for sending scancodes related to button presses and movement.
* First, this function will set up the gpio pins on the pi as inputs with pullup resistors for the 
* clock and data to read ones and zeros for reading scancodes. Next, the interrupts are enabled and
* initialized to detect when a bit is sent from the mouse. Next, the function delays to ensure that
* the mouse is ready to recieve a scancode. The function will write the reset scancode to the mouse,
* will ensure that it reads the self-test (BAT) scancode and then the mouse id (0x00) scancode from
* the mouse. If these fail, the function returns false. Then, the scancode for data reporting is sent
* to the mouse. (NOTE: for some reason, the mouses that I have used all have acted differently. One of
* the mouses does not send back actual scancodes until data reporting is sent. The other mouse (which 
* is the more relaible mouse) does not send ack scancodes. This is why no acknowledgement scancodes 
* are read)
*/
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

/*
* This function is used to read a full event from the mouse. This takes in the three scancodes that the mouse
* sends per event and puts them into a mouse_event_t variable. In this, the first scancode is spliced between
* the three bits for the buttons and the overflow bits. The bits from this scancode that represent the sign of
* dx and dy are then used to set the mouse event dx and dy (which are scancodes two and three respectively) to
* positive or negative (based on twos compliment). This function returns that event.
*/
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

/*
* This function just spins until the ringbuffer dequeues a scancode. This scancode is then returned.
*/
int mouse_read_scancode(void)
{
    while (1) {
        int mouseCode = 0;
        if (rb_dequeue(rb, &mouseCode)) return mouseCode;  
    }
}

/*
* This function writes a scancode (the data) to the mouse. Interrupts are disabled since a falling clock
* edge is used to send data bits. This function will set the clock and the data to outputs. After pulling
* the clock pin to low for 100 us, the data pin is pulled low to signal a start. Then the clock is reset 
* to be input, and the falling clock edges are read. This function will write the data bit by bit in little
* endian format to the mouse, and will then send a parity bit. After this, the data pin is set to input
* and we spin until we recieve an ack bit from the mouse. Finally, the interrupts are reset.
*/
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

//These variables keep track of parts of the handler
static int correct_bit = 0;
static int bit_num = 0;
static int scancode = 0;
static int numOnes = 0;
/*
* This function will read in the bit that is sent along the data line along with a falling clock
* edge. This function will keep track of the data/scancode as well as checking for stop, parity, and
* start bits. If any of these fail, the funciton will reset. 
*/
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
