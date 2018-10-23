#include "assert.h"
#include "printf.h"
#include <stddef.h>
#include "strings.h"
#include "uart.h"

static void test_memset(void)
{
    int num1 = 0xefefefef;
    int num2;

    memset(&num2, 0xef, sizeof(int));
    assert(num1 == num2);
}

static void test_memcpy(void)
{
    int num1 = 0x12345678;
    int num2;

    memcpy(&num2, &num1, sizeof(int));
    assert(num1 == num2);
}

static void test_strlen(void)
{
    assert(strlen("green") == 5);
}

static void test_strcmp(void)
{
    assert(strcmp("apples", "apples") == 0);
}

static void test_strlcat(void)
{
    char buf[20] = "CS";
    strlcat(buf, "107e", 20);
    assert(strcmp(buf, "CS107e") == 0);
}

static void test_strtonum(void)
{
    const char *input = "013", *rest;

    int val = strtonum(input, &rest);
    assert(val == 13 && rest == (input + strlen(input)));

    const char *input2 = "0x15fc2hyz", *rest2;
    
    int val2 = strtonum(input2, &rest2);
    assert(val2 == 90050 && rest2 == (input2 + strlen(input2) - 3));

    const char *input3 = "a", *rest3;

    int val3 = strtonum(input3, &rest3);
    assert(val3 == 0 && rest3 == (input3));

    const char *input4 = "14530ag", *rest4;
    
    int val4 = strtonum(input4, &rest4);
    assert(val4 == 14530 && rest4 == (input4)+5);
}


// These aren't part of printf public interface, we must declare them here to
// be able to use them in this test file.
int unsigned_to_base(char *buf, int n, unsigned int val, int base, int min_width);
int signed_to_base(char *buf, int n, int val, int base, int min_width);

static void test_to_base(void)
{
    char buf[5];
int n = unsigned_to_base(buf, 20, 165488, 10, 10);
//    int n = signed_to_base(buf, 5, -9999, 10, 6);
    assert(strcmp(buf, "-099") == 0 && n == 6);
}

static void test_snprintf(void)
{
    char buf[100];
    int bufsize = sizeof(buf);

    // Start off simple...
    snprintf(buf, bufsize, "Hello, world!");
    assert(strcmp(buf, "Hello, world!") == 0);

    // Decimal
    snprintf(buf, bufsize, "%d", 45);
    assert(strcmp(buf, "45") == 0);

    // Hexadecimal
    snprintf(buf, bufsize, "%04x", 0xef);
    assert(strcmp(buf, "00ef") == 0);

    // Pointer
    snprintf(buf, bufsize, "%p", (void *) 0x20200004);
    assert(strcmp(buf, "0x20200004") == 0);

    // Character
    snprintf(buf, bufsize, "%c", 'A');
    assert(strcmp(buf, "A") == 0);

    // String
    snprintf(buf, bufsize, "%s", "binky");
    assert(strcmp(buf, "binky") == 0);

    // Format string with intermixed codes
    snprintf(buf, bufsize, "CS%d%c!", 107, 'e');
    assert(strcmp(buf, "CS107e!") == 0);

    // Test return value
    assert(snprintf(buf, bufsize, "Hello") == 5);
    assert(snprintf(buf, 2, "Hello") == 5);
}


void main(void)
{
    // TODO: Add more and better tests!

    uart_init();
    test_memset();
    test_memcpy();
    test_strlen();
    test_strcmp();
    test_strlcat();
    test_strtonum();
    test_to_base();
//    test_snprintf();
}
