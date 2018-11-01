#include "printf.h"
#include "uart.h"
#include "strings.h"

static void print_triangle(int nlevels)
{
    int size = 1 << nlevels;

    for (int row = size - 1; row >= 0; row--) {
        for (int col = 0; col < row; col++) {
            printf(" ");
        }
        for (int col = 0; col + row < size; col++) {
            printf("%c ", (col & row) ? ' ' : '*');
        }
        printf("\n");
    }
}

void main(void)
{
    uart_init();
    print_triangle(5);
    printf("We have %03d days until %s!", 6, "Halloween");
    printf("");
    int * pizza = (int *)0x20200008;
    printf("\nMy %x pizzas are at address %p", 0x67, pizza);
    char buf[10];
    printf ("\n%c, %c\n",*(buf - 1), *(buf + 10));
    snprintf(buf, 10, "%020x", 5);
    printf ("\n%c, %c\n",*(buf - 1), *(buf + 10));
    printf ("\n%s", buf);
    char ** test;
    printf ("\n%d", strtonum("m-21", test));
    printf ("\n%s", *test);
}
