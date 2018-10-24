#include "printf.h"
#include <stdarg.h>
#include "strings.h"
#include "uart.h"

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
    char storeString[1024];
    memcpy(storeString, arg, 1024);
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
    numArg(count, placeholder, 16, width, 0, arg, buf, bufsize);
}

int vsnprintf(char *buf, int bufsize, const char *format, va_list args) 
{
    int equalPoint = 0;
    int i = 0;
    int count = 0;
    while (format[equalPoint] != '\0') {
        if (format[equalPoint] == '%') {
            const char * temp = NULL;
            int width = strtonum(&format[equalPoint + 1], &temp);
            format = temp;
            if (format[0] == 'd') {
                equalPoint = 0;
                numArg(&count, &i, 10, width, va_arg(args, int), 0, buf, bufsize); 
            } else if (format[0] == 'x') {
                equalPoint = 0;
                numArg(&count, &i, 16, width, 0, va_arg(args, unsigned int), buf, bufsize);
            } else if (format[0] == 'p') {
                equalPoint = 0;
                pointerArg(&count, &i, buf, width, bufsize, (unsigned int)va_arg(args, void*)); 
            } else if (format[0] == 'c') {
                equalPoint = 0;
                if (i < bufsize) buf[i] = (char)va_arg(args, int);
                count++;
            } else if (format[0] == 's') {
                stringArg(&count, & i, va_arg(args, char*), buf, bufsize);
                equalPoint = 0;
            }
        } else {
            if (i < bufsize) buf[i] = format[equalPoint];
            count++;
        }
        i++;
        equalPoint++;
    }
    buf[i] = '\0';
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
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    int count = vsnprintf(buf, 1024, format, ap);
    int totalSize = strlen(buf);
    for (int i = 0; i < totalSize; i++) {
        uart_putchar(buf[i]);
    }
    va_end(ap);
    return count;
}
