#include "strings.h"

void *memset(void *s, int c, size_t n)
{
    unsigned char* mem = (unsigned char*)s;
    for (int i = 0; i < n; i++) {
       mem[i] = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char* dstMem = (unsigned char*)dst;
    unsigned char* srcMem = (unsigned char*)src;
    for (int i = 0; i < n; i++) {
        dstMem[i] = srcMem[i];
    }
    return dst;
}

int strlen(const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) ;
    return i;
}

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

int strlcat(char *dst, const char *src, int maxsize)
{
    int dstSizeInitial = strlen(dst);
    for (int i = 0; i < maxsize - dstSizeInitial - 1; i++) {
        dst[dstSizeInitial + i] = src[i];
        if (src[i] == '\0') return strlen(dst);
    }
    dst[maxsize - dstSizeInitial] = '\0';
    return strlen(dst);
}

static unsigned int numToBeMultiplied(int i, int base) {
    int num = 1;
    while (i != 0) {
        num = num * base;
        i--;
    }
    return num;
}

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

unsigned int strtonum(const char *str, const char **endptr)
{
    int lastNum = 0;
    volatile int base = 10;
    int initLength = strlen(str);
    for (int i = 0; i < initLength; i++) {
        if (str[0] == '0' && str[1] == 'x') {
            base = 16;
            if (i > 1 && (str[i] < '0' || (str[i] > '9' && str[i] < 'A') || (str[i] > 'F' && str[i] < 'a') || str[i] > 'f')) {
                lastNum = i - 1;
                *endptr = &str[i];
                break;
            }
        } else {
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
