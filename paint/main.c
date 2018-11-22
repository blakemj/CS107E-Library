#include "printf.h"
#include "uart.h"
#include "mouse.h"

void main(void) 
{
  int count = 0;
  while(1) {
      count++;
      if (mouse_init()) break;
//      printf("%d", count);
  }
  printf("out"); 
  while (1) {
      mouse_event_t evt = mouse_read_event();
      printf("\n%d, %d, left:%d, mid:%d, right:%d\n", evt.dx, evt.dy, (int)evt.left, (int)evt.middle, (int)evt.right);
  }
}
