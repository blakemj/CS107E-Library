#include "printf.h"
#include <stdarg.h>
#include "strings.h"
#include "uart.h"

//These are constants for the different bases, and the max string output length
#define BASE_TEN 10
#define BASE_SIXTEEN 16
#define MAX_OUTPUT_LEN 1024

/*
* This function is called to find the character value for the one's place of the
* digit passed in (val). The value is found by modding it with the given base (decimal
* or hex) which then corresponds to a given character value 0-9 or a-f. The function
* then returns this value
*/
static char find_value(unsigned int val, int base) {
    char value = 0;
        if (val % base < BASE_TEN) {
            value = val % base + '0';
        } else {
            value = val % base + 'a' - BASE_TEN; //The base ten is subtracted since the first value == decimal 10
        }
    return value;
}

/*
* This function will reshift everything inside of the buf string. This is done when the amount of characters
* reaches the bufsize, and then there is an overflow. This shifts all the characters so the most significant
* digits are saved, and leaves the first spot open for the next most significant character to fill.
*/
static void reshift(char * buf, int size, int sizeOverflow) {
    char temp = buf[0];
    for (int i = 1; i < size - sizeOverflow; i++) {
        char save = buf[i];
        buf[i] = temp;
        temp = save;
    }
}

/*
* This function is called to check to see if there is aneed for a shift by checking the size against the buffer
*/
static void need_for_reshift(char * buf, int * size, int * sizeOverflow, int bufsize) {
    if (*size >= bufsize) {
        (*sizeOverflow)++;
        reshift(buf, *size, *sizeOverflow);
    }
}

/*
* This function is called to loop through the value (val) passed in and check the one's place to be put into 
* the string. Every loop will divide by the base so that the ones place of val is the next most significant
* digit from val before the division. This is put into the buf string from right to left.
*/
static void put_in_char(char *buf, int bufsize, unsigned int val, int base, int * size, int * sizeOverflow) {
    while (val) {
        (*size)++;
        need_for_reshift(buf, size, sizeOverflow, bufsize);
        buf[bufsize - 1 - *size + *sizeOverflow] = find_value(val, base);
        val = val / base;
    }
}

/*
* This function is called to put 0's in front of the number if the width is greater than the size of 
* the number. This function is called whether or not the bufsize has been reached. If it has, it will
* put zeros as the most significant digits, and shift over the number. The least significant digits will
* be lost.
*/
static void move_for_width(char * buf, int * size, int * sizeOverflow, int bufsize, int min_width) {
    while (*size < min_width) {
        (*size)++;
        need_for_reshift(buf, size, sizeOverflow, bufsize);
        buf[bufsize - 1 - *size + *sizeOverflow] = '0';
    }
}

/*
* This function is called to change an unsigned integer into a string of a given base. The 
* function overall works from right to left, so the string is filled at the end of the 
* bufsize starting with the end character. A size variable is created that keeps track
* of the size that the number should be given a min_width, and a sizeOverflow variable is
* used to keep track of how many characters do not fit within the bufsize. At the end, 
* everything is shifted back to the front of the buf string, and the size that the string
* SHOULD be is returned (may be different if the bufsize cut off the string).
*/
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

/*
* This function is used to turn a signed integer into a string. If the integer is negative,
* then the string will begin with a - sign to indicate negative, then the absolute value
* of the number is sent to the unsigned_to_base function. The symbol will take up the first
* spot in the buf string, so all the sizes are different by 1. If the integer is positive, 
* then it simply acts as if it is an unsigned integer.
*/
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

/*
* When an integer is turned into the string, it will take up some of the spaces within the buf array.
* This function is used to fix the placeholder for the array so that it will again know where to place the
* next character. This will never go past the spot before the final spot in the array.
*/
static int fixBufPlacement(int placeholder, int numSize, int bufsize) {
    if (numSize < bufsize - 1 - placeholder) {
        placeholder = placeholder + numSize - 1;
    } else {
        placeholder = bufsize - 2;
    }
    return placeholder;
}

/*
* This function is called when a string argument is needed. This will copy as much as a string that is
* given into the buf string as possible. This will also change the placeholder to be at the next open 
* character in the buf string, and the count will count how many spaces the string SHOULD take up. This
* count may be different if the bufsize is reached.
*/
static void stringArg(int * count, int * placeholder, char * arg, char * buf, int bufsize) {
    char storeString[MAX_OUTPUT_LEN];
    memcpy(storeString, arg, MAX_OUTPUT_LEN);
    int stringSize = strlen(storeString);
    if (*placeholder + stringSize - 1 < bufsize - 1) {
        memcpy(&buf[*placeholder], storeString, stringSize);
        *placeholder = *placeholder + stringSize - 1;
    } else {
        memcpy(&buf[*placeholder], storeString, bufsize - 1 - *placeholder);
        *placeholder = bufsize - 2;
    }
    *count = *count + stringSize;
}

/*
* This function is called when any number argument is needed (unsigned/signed int, hex, decimal, pointer address)
* to be put into the buf string. Depending on if the int is signed or unsigned, it will call the correct respective
* to base function, and will fill in as much of what is left of the buf array as possible for the number. This can
* also have a width attached to it. This will then update the placeholder and the count accordingly (see stringArg 
* for more detail).
*/
static void numArg(int * count, int * placeholder, int base, int width, int givenInt, unsigned int givenUnsigned, char * buf, int bufsize) {
    int numSize = 0;
    if (givenInt == 0) numSize = unsigned_to_base(&buf[*placeholder], bufsize - 1 - *placeholder, givenUnsigned, base, width);
    if (givenUnsigned == 0) numSize = signed_to_base(&buf[*placeholder], bufsize - 1 - *placeholder, givenInt, base, width);
    *placeholder = fixBufPlacement(*placeholder, numSize, bufsize);
    *count = *count + numSize;
}

/*
* This function is called when a pointer address is needed to be placed into the buf string. This ensures that the
* address starts with '0x' and that the number portion (unsigned int) is placed into the correct place in the string
* in the correct hex base. Then, the count and placeholder are shifted 2 for the two extra characters and then it
* becomes a numArg.
*/ 
static void pointerArg(int * count, int * placeholder, char * buf, int width, int bufsize, unsigned int arg) {
    buf[*placeholder] = '0';
    buf[*placeholder + 1] = 'x';
    *placeholder = *placeholder + 2;
    *count = *count + 2;
    numArg(count, placeholder, BASE_SIXTEEN, width, 0, arg, buf, bufsize);
}

/*
* This function is called to determine which type of code argument to do. This will also fix the equalPoint if there 
* actually wasn't an argument code being called after the '%'. The respective argument will do what is necessary.
*/
static void do_code(const char * format, char * buf, int bufsize, int width, va_list * args, int * count, int * placeholder, int *  equalPoint) {
    int notCode = *equalPoint;
    *equalPoint = 0;
    if (format[0] == 'd') {
        numArg(count, placeholder, BASE_TEN, width, va_arg(*args, int), 0, buf, bufsize);
    } else if (format[0] == 'x') {
        numArg(count, placeholder, BASE_SIXTEEN, width, 0, va_arg(*args, unsigned int), buf, bufsize);
    } else if (format[0] == 'p') {
        pointerArg(count, placeholder, buf, width, bufsize, (unsigned int)va_arg(*args, void*));
    } else if (format[0] == 'c') { //This will simply place the character as the next character in the string.
        if (*placeholder < bufsize - 1) buf[*placeholder] = (char)va_arg(*args, int);
        count++;
    } else if (format[0] == 's') {
        stringArg(count, placeholder, va_arg(*args, char*), buf, bufsize);
    } else {
        *equalPoint = notCode;
    }
}

/*
* This function will take in a list of arguments and a format string and give back the wanted buf string.
* This will read through the format string and copy it into the buf string until a format code is found.
* It will then do as the format code needs. It will also check within the format code to see if a min_width
* was given for the number arguments. It will then call do_code to determine what is needed to be done,
* then at the end, it will update the placeholder and the equalPoint (aka the figure placeholder at an "equal
* point" within the string). Then it will set the last character in the buf string to '\0' to signal the end
* of the string. This will return how many characters SHOULD have been put into the string (even if the bufsize
* is reached). 
*/
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
            if (placeholder < bufsize - 1) {
                buf[placeholder] = format[equalPoint];
            } else {
                placeholder--;
            }
            count++;
        }
        placeholder++;
        equalPoint++;
    }
    buf[placeholder] = '\0';
    return count;  
}

/*
* This function will take in a variable amount of arguments depending on what format codes
* are called for within the format string, and will create a va_list of these. This will then
* call for vsnprintf to create a buf string.
*/
int snprintf(char *buf, int bufsize, const char *format, ...) 
{
    va_list ap;
    va_start(ap, format);
    int count = vsnprintf(buf, bufsize, format, ap);
    va_end(ap);
    return count;
}

/*
* This fucntion will take in a format string and a variable number of arguments for the codes within
* the format string and will output the desired string. This will actually return the number of characters
* the string SHOULD have (may be different due to bufsize), but it will print to the console the string.
*/
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
