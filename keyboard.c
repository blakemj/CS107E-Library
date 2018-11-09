#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"

//This defines how many bits represent the data being sent
#define NUM_DATA_BITS 8

#define MOVE_TO_START_CODE 0xfa
#define MOVE_TO_END_CODE 0xfe
#define DELETE_LINE_CODE 0xfd

//This defines the pins for the clock and data
const unsigned int CLK  = GPIO_PIN23;
const unsigned int DATA = GPIO_PIN24; 

//This sets up a global variable to keep track of the modifiers for every character
static unsigned int saved_modifiers = 0;
//This keeps track of the key that was previously pressed
static unsigned char prv_key = PS2_KEY_NONE;

//This waits until a falling clock edge occurs, representing the time to read a bit
void wait_for_falling_clock_edge() {
    while (gpio_read(CLK) == 0) {}
    while (gpio_read(CLK) == 1) {}
}

//This initializes the pins as pullup input pins for the clock and data
void keyboard_init(void) 
{
    gpio_set_input(CLK); 
    gpio_set_pullup(CLK); 
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA); 
}

/*
* This function will use the clock and data inputs in order to read a scancode
* being sent from the keyboard. The function waits until the clock has a falling
* edge, and when it does, it will read the first bit, which should be a start bit
* which is always low. If it is a start bit, it will then read all of the data bits
* one by one, waiting for a falling clock edge before each one. After saving these
* (and placing them into an int to represent the character code being read; they are
* placed in the correct digit as they are passed in least significant bit first) it 
* will check that the number of ones that have been sent in correspond the the odd
* or even number expected with the parity bit. If it passes this, the scancode then
* checks for a stop bit. If it passes this, it will return the character scancode
* that was read. If any test fails, it will call itself to try again, and will return
* whatever that gives back.
*/
unsigned char keyboard_read_scancode(void)
{   
    wait_for_falling_clock_edge();
    if (gpio_read(DATA) == 0) {
        int binaryDigit = 1;
        int numOnes = 0;
        unsigned int totalChar = 0; 
        //Read 8 Data bits
        for (int i = NUM_DATA_BITS - 1; i >=0; i--) {
            wait_for_falling_clock_edge();
            totalChar = totalChar + binaryDigit * gpio_read(DATA);
            if (gpio_read(DATA) == 1) numOnes++;
            binaryDigit = binaryDigit * 2;
        }
        //Check Parity Bit
        wait_for_falling_clock_edge();
        if (gpio_read(DATA) == 1) numOnes++;
        if (numOnes % 2 != 1) totalChar = keyboard_read_scancode();
        //Check Stop Bit
        wait_for_falling_clock_edge();
        if(gpio_read(DATA) == 0) totalChar =  keyboard_read_scancode();
        return totalChar;
    } else {
        return keyboard_read_scancode();
    }
    return 0;
}

/*
* This function will read the scancodes sent by the keyboard and check for 
* sequences. If the key is coming back up, it will send a PS2_CODE_RELEASE
* code, signaling that it is in a sequence with the next scancode. If the
* key is a special extended code, it will first send a PS2_CODE_EXTEND code
* which will then either be followed by the release code or the code for the
* key. The function will place these code into an array or unsigned characters
* and then will return the length of the sequence.
*/
int keyboard_read_sequence(unsigned char seq[])
{
    seq[0] = keyboard_read_scancode();
    int length = 1;
    int extendedCode = 0;
    if (seq[0] == PS2_CODE_EXTEND) {
        seq[1] = keyboard_read_scancode();
        extendedCode++;
        length++;
    }
    if (seq[extendedCode] == PS2_CODE_RELEASE) {
         seq[extendedCode + 1] = keyboard_read_scancode();
         length++;
    }
    return length;
}

/*
* This function will change the modifiers when a given key event occurs. This
* uses a struct and the gobal variable saved_modifiers. The saved modifiers is
* XOR-ed with the given modifier bit to change it from 1 to 0 or 0 to 1 depending
* on whether or not the modifier was set or not.
*/
static key_event_t change_modifiers(key_event_t event) {
    if(event.key.ch == PS2_KEY_NUM_LOCK) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_NUM_LOCK;
    if(event.key.ch == PS2_KEY_SCROLL_LOCK) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_SCROLL_LOCK;
    if(event.key.ch == PS2_KEY_SHIFT) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_SHIFT;
    if(event.key.ch == PS2_KEY_ALT) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_ALT;
    if(event.key.ch == PS2_KEY_CTRL) saved_modifiers = saved_modifiers ^ KEYBOARD_MOD_CTRL;
    return event;
}

/*
* This function will take in a keyboard sequence and set up a key event struct. This will set the
* event struct for the given event to include the character, set the given modifiers for the key,
* and set whether or not the key was being pressed or released. This sets all the modifers when
* the key is initially pressed down, and clears them when the key is released (except fo the caps
* lock key which will only be cleared when pressed down again). This will return the key event.
*/
key_event_t keyboard_read_event(void) 
{
    key_event_t event;
    event.seq_len = keyboard_read_sequence(event.seq);
    event.key = ps2_keys[event.seq[event.seq_len - 1]];
    if (event.seq[0] == PS2_CODE_EXTEND) {
        if (event.seq[event.seq_len - 1] == 0x75) event.key.ch = PS2_KEY_ARROW_UP;
        if (event.seq[event.seq_len - 1] == 0x6b) event.key.ch = PS2_KEY_ARROW_LEFT;
        if (event.seq[event.seq_len - 1] == 0x72) event.key.ch = PS2_KEY_ARROW_DOWN;
        if (event.seq[event.seq_len - 1] == 0x74) event.key.ch = PS2_KEY_ARROW_RIGHT;
    }
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

/*
* This function will read the next key that the user presses and will return the correct
* version of the character given the modifiers. If the key is being depressed, the funciton
* will call itself to wait for the next key so that it only reads keys as they are being 
* pressed (so you don't get duplicate characters). This will then also read the shift and 
* caps lock modifiers to determine which version of the key character should be returned,
* and then it will return the character. If shift is pressed, it will return the capital
* version of the character (or the secondary special character). If caps lock is pressed,
* it will return the capital of the letters (but the special characters and numbers will
* be unaffected). If control is pressed, then it will check if it is one of the shortcut
* codes (a, e, or u; a for going to the first character on a line, e for going to the last
* character in a line, or u for deleting a whole line.
*/
unsigned char keyboard_read_next(void) 
{
    key_event_t event = keyboard_read_event();
    if (event.action == KEYBOARD_ACTION_UP) return keyboard_read_next();
    if (event.key.ch == PS2_KEY_SHIFT || event.key.ch == PS2_KEY_CAPS_LOCK || event.key.ch == PS2_KEY_CTRL || event.key.ch == PS2_KEY_ALT) return keyboard_read_next();
    char toBeReturned = event.key.ch;
    if (event.key.ch <= 0x7f) {
        if (event.modifiers & KEYBOARD_MOD_SHIFT) toBeReturned = event.key.other_ch;
        if (event.key.ch >= 'a' && event.key.ch <= 'z') {
            if (event.modifiers & KEYBOARD_MOD_CAPS_LOCK) {
                toBeReturned = event.key.other_ch;
            }
            if (event.modifiers & KEYBOARD_MOD_CTRL) {
                if (event.key.ch == 'a') toBeReturned = MOVE_TO_START_CODE;
                if (event.key.ch == 'e') toBeReturned = MOVE_TO_END_CODE;
                if (event.key.ch == 'u') toBeReturned = DELETE_LINE_CODE;
            }
        }
    }
    return toBeReturned;
}
