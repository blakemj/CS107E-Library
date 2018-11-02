#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"

const unsigned int CLK  = GPIO_PIN23;
const unsigned int DATA = GPIO_PIN24; 

static unsigned int saved_modifiers = 0;
static unsigned char prv_key = PS2_KEY_NONE;

void wait_for_falling_clock_edge() {
    while (gpio_read(CLK) == 0) {}
    while (gpio_read(CLK) == 1) {}
}

void keyboard_init(void) 
{
    gpio_set_input(CLK); 
    gpio_set_pullup(CLK); 
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA); 
}

unsigned char keyboard_read_scancode(void)
{   
    wait_for_falling_clock_edge();
    if (gpio_read(DATA) == 0) {
        int binaryDigit = 1;
        int numOnes = 0;
        unsigned int totalChar = 0; 
        for (int i = 7; i >=0; i--) {
            wait_for_falling_clock_edge();
            totalChar = totalChar + binaryDigit * gpio_read(DATA);
            if (gpio_read(DATA) == 1) numOnes++;
            binaryDigit = binaryDigit * 2;
        }
        wait_for_falling_clock_edge();
        if (gpio_read(DATA) == 1) numOnes++;
        if (numOnes % 2 != 1) keyboard_read_scancode();
        wait_for_falling_clock_edge();
        if(gpio_read(DATA) == 0) keyboard_read_scancode();
        return totalChar;
    } else {
        keyboard_read_scancode();
    }
    return 0;
}

int keyboard_read_sequence(unsigned char seq[])
{
    // The implementation started below assumes a sequence is exactly
    // one byte long. Although this is the case for many key actions,
    // is not true for all.
    // What key actions generate a sequence of length 2?  What
    // about length 3?
    // Figure out what those cases are and extend this code to
    // recognize them and read the full sequence.
    seq[0] = keyboard_read_scancode();
    int length = 1;
    int i = 0;
    if (seq[0] == PS2_CODE_EXTEND) {
        seq[1] = keyboard_read_scancode();
        i++;
        length++;
    }
    if (seq[i] == PS2_CODE_RELEASE) {
         seq[i+1] = keyboard_read_scancode();
         length++;
    }
    return length;
}

static key_event_t change_modifiers(key_event_t event) {
    if(event.key.ch == PS2_KEY_NUM_LOCK) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_NUM_LOCK;
    if(event.key.ch == PS2_KEY_SCROLL_LOCK) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_SCROLL_LOCK;
    if(event.key.ch == PS2_KEY_SHIFT) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_SHIFT;
    if(event.key.ch == PS2_KEY_ALT) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_ALT;
    if(event.key.ch == PS2_KEY_CTRL) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_CTRL;
    return event;
}

key_event_t keyboard_read_event(void) 
{
    key_event_t event;
    event.seq_len = keyboard_read_sequence(event.seq);
    event.key = ps2_keys[event.seq[event.seq_len - 1]];
    if (event.seq[event.seq_len - 2] == PS2_CODE_RELEASE) {
        event.action = KEYBOARD_ACTION_UP;
        event = change_modifiers(event);
        prv_key = PS2_KEY_NONE;
    } else {
        event.action = KEYBOARD_ACTION_DOWN;
        if(prv_key != event.key.ch) event = change_modifiers(event);
        prv_key = event.key.ch;
        if(event.key.ch == PS2_KEY_CAPS_LOCK) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_CAPS_LOCK;
    }
    event.modifiers = saved_modifiers;
    return event;
}


unsigned char keyboard_read_next(void) 
{
    key_event_t event = keyboard_read_event();
    if (event.action == KEYBOARD_ACTION_UP) return keyboard_read_next();
    if (event.key.ch == PS2_KEY_SHIFT || event.key.ch == PS2_KEY_CAPS_LOCK) return keyboard_read_next();
    char toBeReturned = event.key.ch;
    if (event.key.ch <= 0x7f) {
        if (event.modifiers & KEYBOARD_MOD_SHIFT) toBeReturned = event.key.other_ch;
        if (event.key.ch >= 'a' && event.key.ch <= 'z') {
            if (event.modifiers & KEYBOARD_MOD_CAPS_LOCK) {
                if (event.modifiers & KEYBOARD_MOD_SHIFT) {
                    toBeReturned = event.key.ch;
                } else {
                    toBeReturned = event.key.other_ch;
                }
            }
        }
    }
    return toBeReturned;
}
