#include "gpio.h"
#include "gpioextra.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "mouse.h"
#include "printf.h"
#include "timer.h"
#include "assert.h"

const unsigned int MOUSE_CLK = GPIO_PIN23;
const unsigned int MOUSE_DATA = GPIO_PIN24;

#define CMD_RESET 0xFF
#define CMD_ENABLE_DATA_REPORTING 0xF4

#define NUM_DATA_BITS 8

static rb_t *rb;

static void mouse_write(unsigned char data);
static void wait_for_falling_mouse_clock_edge();
int mouse_read_scancode();
static void mouse_handler(unsigned int pc);

bool mouse_init(void)
{
  rb = rb_new();

  gpio_set_function(MOUSE_CLK, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_CLK);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);

    rb = rb_new();

    gpio_enable_event_detection(MOUSE_CLK, GPIO_DETECT_FALLING_EDGE);
    bool ok = interrupts_attach_handler(mouse_handler);
    assert(ok);
    interrupts_enable_source(INTERRUPTS_GPIO3);
    interrupts_global_enable();

  mouse_write(CMD_RESET);
  int ack = mouse_read_scancode();
  if (ack != 0xfa) return false;
  int bat = mouse_read_scancode(); 
  if (bat != 0xaa) printf("%x\n", bat);
  int id = mouse_read_scancode(); 
  if (id != 0x00) printf("%x\n", id);
  printf("Made it!");
  mouse_write(CMD_ENABLE_DATA_REPORTING);
  ack = mouse_read_scancode(); 
  if (ack != 0xfa) return false;
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
      evt.dx = scancodeThree;
  }
  // FIXME: Read scancode(s) and fill in evt.

  return evt;
}

static void wait_for_falling_mouse_clock_edge() {
    while (gpio_read(MOUSE_CLK) == 0) {}
    while (gpio_read(MOUSE_CLK) == 1) {}
}

int mouse_read_scancode(void)
{
    while (1) {
        int charCode = 0;
        if (rb_dequeue(rb, &charCode)) {
            return charCode;
        }
    }
}

static void mouse_write(unsigned char data)
{
    interrupts_disable_source(INTERRUPTS_GPIO3);
    gpio_set_output(MOUSE_CLK);
    gpio_set_output(MOUSE_DATA);
    gpio_write(MOUSE_CLK, 0);
    timer_delay_us(120);
//    gpio_write(MOUSE_DATA, 0);
    gpio_write(MOUSE_CLK, 1);
    gpio_set_input(MOUSE_CLK);
    gpio_set_pullup(MOUSE_CLK);
    gpio_write(MOUSE_DATA, 0);
    int numOne = 0;
    for (int i = 0; i < NUM_DATA_BITS; i++) {
        wait_for_falling_mouse_clock_edge();
        gpio_write(MOUSE_DATA, (data & 1));
        if ((data & 1) == 1) numOne++;
        data = data >> 1;
    }
    if (numOne != 8) printf("shit");
    wait_for_falling_mouse_clock_edge();
    if (numOne % 2 == 0) {
        gpio_write(MOUSE_DATA, 1);
    } else {
        gpio_write(MOUSE_DATA, 0);
    }
    wait_for_falling_mouse_clock_edge();
    gpio_set_input(MOUSE_DATA);
    gpio_set_pullup(MOUSE_DATA);
    wait_for_falling_mouse_clock_edge();
    while(gpio_read(MOUSE_DATA) == 0);
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
        printf("%x", bit);
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
  // FIXME: Handle event on mouse clock line
}
