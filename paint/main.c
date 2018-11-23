#include "printf.h"
#include "uart.h"
#include "mouse.h"
#include "gl.h"
#include "timer.h"

static int xCoord = 0;
static int yCoord = 0;
static int count = 0;

void main(void) 
{
  gl_init(500, 500, GL_DOUBLEBUFFER);
  while(1) {
      count++;
      if (mouse_init()) break;
  }
  while (1) {
      gl_draw_rect(xCoord, yCoord, 4, 4, GL_BLACK);
      mouse_event_t evt1 = mouse_read_event();
      mouse_event_t evt2 = mouse_read_event();
      mouse_event_t evt3 = mouse_read_event();
      mouse_event_t evt4 = mouse_read_event();
      mouse_event_t evt5 = mouse_read_event();
      int dx = (evt1.dx + evt2.dx + evt3.dx + evt4.dx + evt5.dx) / 5;
      int dy = (evt1.dy + evt2.dy + evt3.dy + evt4.dy + evt5.dy) / 5;

      if ((dx < 10 || dx > 10) && xCoord + dx > 0 && xCoord + dx < 500) xCoord = xCoord + dx;
      if ((dy < 10 || dy > 10) && yCoord - dy > 0 && yCoord - dy < 500) yCoord = yCoord - dy;
      if (evt1.left || evt2.left || evt3.left || evt4.left || evt5.left) gl_draw_pixel(xCoord - 1, yCoord - 1, GL_RED);
      count++;
      gl_draw_rect(xCoord, yCoord, 4, 4, GL_BLUE);
//      timer_delay_us(25);
      if (count == 10) {
          gl_swap_buffer();
          count = 0;
      }
  }
}
