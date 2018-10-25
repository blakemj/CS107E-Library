#include "printf.h"
#include "uart.h"

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
}
