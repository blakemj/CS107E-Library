#include "printf.h"
#include <stdarg.h>

#define MAX_OUTPUT_LEN 1024

static char find_value(unsigned int val, int base) {
    char value = 0;
        if (val % base < 10) {
            value = val % base + 48;
        } else {
            value = val % base + 87;
        }
    return value;
}

static void reshift(char * buf, int size, int sizeOverflow) {
    char temp = buf[0];
    for (int i = 1; i < size - sizeOverflow; i++) {
        char save = buf[i];
        buf[i] = temp;
        temp = save;
    }
}

int unsigned_to_base(char *buf, int bufsize, unsigned int val, int base, int min_width) 
{
    if (bufsize == 0) {
        return 0;
    } else {
        buf[bufsize - 1] = '\0';
    }
    int size = 0;
    int sizeOverflow = 0;
    while (val) {
        size++;
        if (size >= bufsize) {
            sizeOverflow++;
            reshift(buf, size, sizeOverflow);
        }
        buf[bufsize - 1 - size + sizeOverflow] = find_value(val, base);
        val = val / base;
    }
    while (size < min_width) {
        size++;
        if (size >=bufsize) {
            sizeOverflow++;
            reshift(buf, size, sizeOverflow);
        }
        buf[bufsize - 1 - size + sizeOverflow] = '0';
    }
    if (size < bufsize) {
        for (int i = 0; i <= size; i++) {
            buf[i] = buf[bufsize - 1 - size + i];
        }
    }
    return size - sizeOverflow;
}

int signed_to_base(char *buf, int bufsize, int val, int base, int min_width) 
{
    /* TODO: Your code here */
    return 0;
}

int vsnprintf(char *buf, int bufsize, const char *format, va_list args) 
{
    /* TODO: Your code here */
    return 0;
}

int snprintf(char *buf, int bufsize, const char *format, ...) 
{
    /* TODO: Your code here */
    return 0;
}

int printf(const char *format, ...) 
{
    /* TODO: Your code here */
    return 0;
}
