#include "printf.h"
#include <stdarg.h>
#include "strings.h"
#include "uart.h"

#define BASE_TEN 10
#define BASE_SIXTEEN 16
#define MAX_OUTPUT_LEN 1024

static char find_value(unsigned int val, int base) {
    char value = 0;
        if (val % base < BASE_TEN) {
            value = val % base + '0';
        } else {
            value = val % base + 'a' - BASE_TEN;
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

static void need_for_reshift(char * buf, int * size, int * sizeOverflow, int bufsize) {
    if (*size >= bufsize) {
        (*sizeOverflow)++;
        reshift(buf, *size, *sizeOverflow);
    }
}

static void put_in_char(char *buf, int bufsize, unsigned int val, int base, int * size, int * sizeOverflow) {
    while (val) {
        (*size)++;
        need_for_reshift(buf, size, sizeOverflow, bufsize);
        buf[bufsize - 1 - *size + *sizeOverflow] = find_value(val, base);
        val = val / base;
    }
}

static void move_for_width(char * buf, int * size, int * sizeOverflow, int bufsize, int min_width) {
    while (*size < min_width) {
        (*size)++;
        need_for_reshift(buf, size, sizeOverflow, bufsize);
        buf[bufsize - 1 - *size + *sizeOverflow] = '0';
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
    put_in_char(buf, bufsize, val, base, &size, &sizeOverflow);
    move_for_width(buf, &size, &sizeOverflow, bufsize, min_width);
    if (size < bufsize) {
        for (int i = 0; i <= size; i++) {
            buf[i] = buf[bufsize - 1 - size + i];
        }
    }
    return size;
}

int signed_to_base(char *buf, int bufsize, int val, int base, int min_width) 
{
    int size = 0;
    if (val < 0) {
        buf[0] = '-';
        size =1 +  unsigned_to_base(&buf[1], bufsize - 1, val * -1, base, min_width - 1);
    } else {
        size = unsigned_to_base(buf, bufsize, val, base, min_width);
    }
    return size;
}

static int fixBufPlacement(int placeholder, int numSize, int bufsize) {
    if (numSize < bufsize - 1 - placeholder) {
        placeholder = placeholder + numSize - 1;
    } else {
        placeholder = bufsize - 2;
    }
    return placeholder;
}

static void stringArg(int * count, int * placeholder, char * arg, char * buf, int bufsize) {
    char storeString[MAX_OUTPUT_LEN];
    memcpy(storeString, arg, MAX_OUTPUT_LEN);
    int stringSize = strlen(storeString);
    if (*placeholder + stringSize - 1 < bufsize) (char*)memcpy(&buf[*placeholder], storeString, stringSize);
    *placeholder = *placeholder + stringSize - 1;
    *count = *count + stringSize;
}

static void numArg(int * count, int * placeholder, int base, int width, int givenInt, unsigned int givenUnsigned, char * buf, int bufsize) {
    int numSize = 0;
    if (givenInt == 0) numSize = unsigned_to_base(&buf[*placeholder], bufsize - 1 - *placeholder, givenUnsigned, base, width);
    if (givenUnsigned == 0) numSize = signed_to_base(&buf[*placeholder], bufsize - 1 - *placeholder, givenInt, base, width);
    *placeholder = fixBufPlacement(*placeholder, numSize, bufsize);
    *count = *count + numSize;
}

static void pointerArg(int * count, int * placeholder, char * buf, int width, int bufsize, unsigned int arg) {
    buf[*placeholder] = '0';
    buf[*placeholder + 1] = 'x';
    *placeholder = *placeholder + 2;
    *count = *count + 2;
    numArg(count, placeholder, BASE_SIXTEEN, width, 0, arg, buf, bufsize);
}

static void do_code(const char * format, char * buf, int bufsize, int width, va_list * args, int * count, int * placeholder, int *  equalPoint) {
    int notCode = *equalPoint;
    *equalPoint = 0;
    if (format[0] == 'd') {
        numArg(count, placeholder, BASE_TEN, width, va_arg(*args, int), 0, buf, bufsize);
    } else if (format[0] == 'x') {
        numArg(count, placeholder, BASE_SIXTEEN, width, 0, va_arg(*args, unsigned int), buf, bufsize);
    } else if (format[0] == 'p') {
        pointerArg(count, placeholder, buf, width, bufsize, (unsigned int)va_arg(*args, void*));
    } else if (format[0] == 'c') { 
        if (*placeholder < bufsize) buf[*placeholder] = (char)va_arg(*args, int);
        count++;
    } else if (format[0] == 's') {
        stringArg(count, placeholder, va_arg(*args, char*), buf, bufsize);
    } else {
        *equalPoint = notCode;
    }
}

int vsnprintf(char *buf, int bufsize, const char *format, va_list args) 
{
    int equalPoint = 0;
    int placeholder = 0;
    int count = 0;
    while (format[equalPoint] != '\0') {
        if (format[equalPoint] == '%') {
            int width = 0;
            if (format[equalPoint + 1] == '0') {
                const char * temp = NULL;
                width = strtonum(&format[equalPoint + 1], &temp);
                format = temp;
            } else {
                format = &format[equalPoint + 1];
            }
            do_code(format, buf, bufsize, width, &args, &count, &placeholder, &equalPoint);
        } else {
            if (placeholder < bufsize) buf[placeholder] = format[equalPoint];
            count++;
        }
        placeholder++;
        equalPoint++;
    }
    buf[placeholder] = '\0';
    return count;  
}

int snprintf(char *buf, int bufsize, const char *format, ...) 
{
    va_list ap;
    va_start(ap, format);
    int count = vsnprintf(buf, bufsize, format, ap);
    va_end(ap);
    return count;
}

int printf(const char *format, ...) 
{
    char buf[MAX_OUTPUT_LEN];
    va_list ap;
    va_start(ap, format);
    int count = vsnprintf(buf, MAX_OUTPUT_LEN, format, ap);
    int totalSize = strlen(buf);
    for (int i = 0; i < totalSize; i++) {
        uart_putchar(buf[i]);
    }
    va_end(ap);
    return count;
}
