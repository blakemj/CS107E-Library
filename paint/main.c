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
      mouse_event_t evt1 = mouse_read_event();
      mouse_event_t evt2 = mouse_read_event();
      mouse_event_t evt3 = mouse_read_event();
      mouse_event_t evt4 = mouse_read_event();
      mouse_event_t evt5 = mouse_read_event();
      int dx = (evt1.dx + evt2.dx + evt3.dx + evt4.dx + evt5.dx) / 5;
      int dy = (evt1.dy + evt2.dy + evt3.dy + evt4.dy + evt5.dy) / 5;
      if (dx < 25 && dx > -25 && xCoord + dx > 0 && xCoord + dx < 500) xCoord = xCoord + dx;
      if (dy < 25 && dy > -25 && yCoord - dy > 0 && yCoord - dy < 500) yCoord = yCoord - dy;
      if (evt1.left || evt2.left || evt3.left || evt4.left || evt5.left) {
          if (xCoord > 1 && yCoord > 1) gl_draw_pixel(xCoord - 1, yCoord - 1, GL_RED);
          if (xCoord > 2 && yCoord > 2) gl_draw_pixel(xCoord - 2, yCoord - 2, GL_RED);
          if (xCoord > 1 && yCoord > 2) gl_draw_pixel(xCoord - 1, yCoord - 2, GL_RED);
          if (xCoord > 2 && yCoord > 1) gl_draw_pixel(xCoord - 2, yCoord - 1, GL_RED);
      }
      count++;
      color_t saveRect[4][4];
      for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++) {
             saveRect[i][j] = gl_read_pixel(xCoord + i, yCoord + j);
          }  
      } 
      gl_draw_rect(xCoord, yCoord, 4, 4, GL_BLUE);
      if (count == 4) {
          gl_swap_buffer();
          count = 0;
      }
      for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++) {
             gl_draw_pixel(xCoord + i, yCoord + j, saveRect[i][j]);
          }
      } 
  }
}
