#include "printf.h"
#include <stdarg.h>
#include "strings.h"

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

int vsnprintf(char *buf, int bufsize, const char *format, va_list args) 
{
    /* TODO: Your code here */
    return 0;
}

static int fixBufPlacement(int placeholder, int numSize, int bufsize) {
    if (numSize < bufsize - 1 - placeholder) {
        placeholder = placeholder + numSize - 1;
    } else {
        placeholder = bufsize - 2;
    }
    return placeholder;
}

int snprintf(char *buf, int bufsize, const char *format, ...) 
{
    int equalPoint = 0;
    va_list ap;
    va_start(ap, format);
    int i = 0;
    int count = 0;
    while (format[equalPoint] != '\0') {
        if (format[equalPoint] == '%') {
            const char * temp = NULL;
            int width = strtonum(&format[equalPoint + 1], &temp);
            format = temp;
            if (format[0] == 'd') {
                equalPoint = 0;
                int numSize =  signed_to_base(&buf[i], bufsize - 1 - i,va_arg(ap, int), 10, width);
                i = fixBufPlacement(i, numSize, bufsize);
                count = count + numSize;
            } else if (format[0] == 'x') {
                equalPoint = 0;
                int numSize =  signed_to_base(&buf[i], bufsize - 1 - i,va_arg(ap, unsigned int), 16, width);
                i = fixBufPlacement(i, numSize, bufsize);
                count = count + numSize;
            } else if (format[0] == 'p') {
                equalPoint = 0;
                buf[i] = '0';
                buf[i+1] = 'x';
                int numSize = signed_to_base(&buf[i+2], bufsize - 3 - i, (int)va_arg(ap, void*), 16, 8);
                i = fixBufPlacement(i + 2, numSize, bufsize);
                count = count + 2 + numSize;
            } else if (format[0] == 'c') {
                equalPoint = 0;
                if (i < bufsize) buf[i] = (char)va_arg(ap, int);
                count++;
            } else if (format[0] == 's') {
                equalPoint = 0;
                char storeString[1024];
                (char*)memcpy(storeString, va_arg(ap, char*), 1024);
                int stringSize = strlen(storeString);
                if (i + stringSize - 1 < bufsize) (char*)memcpy(&buf[i], storeString, stringSize);
                i = i + stringSize - 1;
                count = count + stringSize;
            }
        } else {
            if (i < bufsize) buf[i] = format[equalPoint];
            count++;
        }
        i++;
        equalPoint++;
    }
    va_end(ap);
    buf[i] = '\0';
    return count;
}

int printf(const char *format, ...) 
{
    /* TODO: Your code here */
    return 0;
}
