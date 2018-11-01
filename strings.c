#include "strings.h"

/*
* This function will set n amount of data in s pointer to the int c. This will return the s pointer.
*/
void *memset(void *s, int c, size_t n)
{
    unsigned char* mem = (unsigned char*)s;
    for (int i = 0; i < n; i++) {
       mem[i] = (unsigned char)c;
    }
    return s;
}

/*
* This function will take in a src memory pointer and will copy n amount of data over to 
* the dst pointer. This will then return the dst pointer.
*/
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char* dstMem = (unsigned char*)dst;
    unsigned char* srcMem = (unsigned char*)src;
    for (int i = 0; i < n; i++) {
        dstMem[i] = srcMem[i];
    }
    return dst;
}

/*
* This function will simply return the length of a string.
*/
int strlen(const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) ;
    return i;
}

/*
* This function will take in two strings and compare them. If they are equal, it will return 0. If s1 is larger
* than s2 (based on ascii values), then it will return 1. If the opposite is true, then it will return -1.
*/
int strcmp(const char *s1, const char *s2)
{
    int s1Length = strlen(s1);
    for (int i = 0; i < s1Length; i++) {
        if (s2[i] == '\0'|| s1[i] > s2[i]) return 1;
        if (s1[i] < s2[i]) return -1;
    }
    if (s2[s1Length] == '\0') return 0;
    return -1;
}

/*
* This function will append the src string to the dst string up until a maxsize. The length of this string is then returned.
*/
int strlcat(char *dst, const char *src, int maxsize)
{
    int dstSizeInitial = strlen(dst);
    for (int i = 0; i < maxsize - dstSizeInitial; i++) {
        dst[dstSizeInitial + i] = src[i];
        if (src[i] == '\0') return strlen(dst);
    }
    dst[maxsize - 1] = '\0';
    return strlen(dst);
}

/*
* This funtion will multiply a number by its base a number of times until the digit holds its necessary value.
*/
static unsigned int numToBeMultiplied(int i, int base) {
    int num = 1;
    while (i != 0) {
        num = num * base;
        i--;
    }
    return num;
}

/*
* This function will loop through all of the characters in the string and determine the characters value as a hex or decimal
* number. A running total is kept, but each digit is added after being passed into the numToBeMultiplied function which will
* multiply it by its base the number of times necessary given its place (for instance the 1 in 100 is larger than the 1 in 10).
* This function returns that total.
*/
static unsigned int findingNum(const char *str, int lastNum, int base) {
    unsigned int total = 0;
    for (int a = 0; a <= lastNum; a++) {
        char charMult = '\0';
        if (str[a] <= '9') charMult = str[a] - '0';
        if (str[a] >= 'a' && str[a] <= 'f') charMult = str[a] - 'a' + 10;
        if (str[a] >= 'A' && str[a] <= 'F') charMult = str[a] - 'A' + 10;
        total = total + (charMult * numToBeMultiplied(lastNum - a, base));
    }
    return total;
}

/*
* This function is called to take a string and convert it to a number. If the string has some non-number components
* to it after the number, a pointer will point to this, and the number will be returned. This will be able to take in
* a hex number of any case as well.
*/
unsigned int strtonum(const char *str, const char **endptr)
{
    int lastNum = 0;
    volatile int base = 10;
    int initLength = strlen(str);
    for (int i = 0; i < initLength; i++) {

        //This part is for when a number is a hex number
        if (str[0] == '0' && str[1] == 'x') {
            base = 16;
            //This will check to see if the character is a hex number; if not, the endptr will point to it, and it will break from the loop
            if (i > 1 && (str[i] < '0' || (str[i] > '9' && str[i] < 'A') || (str[i] > 'F' && str[i] < 'a') || str[i] > 'f')) {
                lastNum = i - 1;
                *endptr = &str[i];
                break;
            }
        //This part of string to num is when a string is not a hex number
        } else {
            //If the string is a decimal number, it will enter this and break from the loop once a non-decimal character is reached
            if (str[i] < '0' || str[i] > '9') {
                lastNum = i - 1;
                *endptr = &str[i];
                break;
            }
        }
        lastNum = i;
        *endptr = &str[i + 1];
    }
    return findingNum(str, lastNum, base);
}
