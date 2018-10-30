#include "keyboard.h"
#include "printf.h"
#include "uart.h"
#include "timer.h"

#define ESC_SCANCODE 0x76

static void test_keyboard_scancodes(void) 
{
    printf("\nNow reading single scancodes. Type ESC to finish this test.\n");
    while (1) {
        unsigned char scancode = keyboard_read_scancode();
        printf("%02x\n", scancode);
        if (scancode == ESC_SCANCODE) break;
    }
    printf("Done with scancode test.\n");
}

static void test_keyboard_sequences(void)
{
    printf("\nNow reading scancode sequences. Type ESC to finish this test.\n");
    while (1) {
        unsigned char seq[3];
        int len = keyboard_read_sequence(seq);
        printf("Scan code sequence: ");
        for (int i = 0; i < len; i++)
            printf("%02x ", seq[i]);
        printf("\n");
        if (seq[0] == ESC_SCANCODE) break;
    }
    printf("Done with scancode sequences test.\n");
}

static void test_keyboard_events(void) 
{
    printf("\nNow reading key events. Type ESC to finish this test.\n");
    while (1) {
        key_event_t evt = keyboard_read_event();
        printf("%s Key: [%02x] Modifiers: 0x%x\n", evt.action == KEYBOARD_ACTION_DOWN? "Down" : "Up  ", evt.key.ch, evt.modifiers);
        if (evt.key.ch == ps2_keys[ESC_SCANCODE].ch) break;
    }
    printf("Done with key events test.\n");
}

static void test_keyboard_chars(void) 
{
    printf("\nNow reading chars. Type ESC to finish this test.\n");
    while (1) {
        char c = keyboard_read_next();
        if (c >= '\t' && c <= 0x80) 
            printf("%c", c);
        else
            printf("[%02x]", c);
        if (c == ps2_keys[ESC_SCANCODE].ch) break;
    }
    printf("\nDone with key chars test.\n");
}

void main(void) 
{
    uart_init();
    keyboard_init();

    printf("Testing keyboard module.\n");

    test_keyboard_scancodes();
    timer_delay_ms(500);

    test_keyboard_sequences();
    timer_delay_ms(500);

    test_keyboard_events();
    timer_delay_ms(500);

    test_keyboard_chars();

    printf("All done!\n\04");
}
