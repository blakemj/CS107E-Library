#include "printf.h"
#include "uart.h"
#include "mouse.h"
#include "gl.h"
#include "timer.h"

//These define the screen size and the range of allowed mouse displacement
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define DISPLACEMENT_RANGE 25

//These variables keep track of the coordinates and the count
static int xCoord = 0;
static int yCoord = 0;
static int count = 0;

/*
* This function will read 5 events from the mouse, then will average the x and y displacements (this
* ensures that outliers are muted slightly). If these displacements are within the ranges of allowed values
* and is within the screen, then the x and y coordinates are updated. Finally, if during any of those events
* the left cursor was pressed, then a red dot is drawn to paint the screen.
*/
static void readCursor() {
      mouse_event_t evt1 = mouse_read_event();
      mouse_event_t evt2 = mouse_read_event();
      mouse_event_t evt3 = mouse_read_event();
      mouse_event_t evt4 = mouse_read_event();
      mouse_event_t evt5 = mouse_read_event();
      int dx = (evt1.dx + evt2.dx + evt3.dx + evt4.dx + evt5.dx) / 5;
      int dy = (evt1.dy + evt2.dy + evt3.dy + evt4.dy + evt5.dy) / 5;
      if (dx < DISPLACEMENT_RANGE && dx > -DISPLACEMENT_RANGE && xCoord + dx > 0 && xCoord + dx < SCREEN_WIDTH) xCoord = xCoord + dx;
      if (dy < DISPLACEMENT_RANGE && dy > -DISPLACEMENT_RANGE && yCoord - dy > 0 && yCoord - dy < SCREEN_HEIGHT) yCoord = yCoord - dy;
      if (evt1.left || evt2.left || evt3.left || evt4.left || evt5.left) {
          if (xCoord > 1 && yCoord > 1) gl_draw_pixel(xCoord - 1, yCoord - 1, GL_RED);
          if (xCoord > 2 && yCoord > 2) gl_draw_pixel(xCoord - 2, yCoord - 2, GL_RED);
          if (xCoord > 1 && yCoord > 2) gl_draw_pixel(xCoord - 1, yCoord - 2, GL_RED);
          if (xCoord > 2 && yCoord > 1) gl_draw_pixel(xCoord - 2, yCoord - 1, GL_RED);
      }
      count++;
}

/*
* In main, the graphics screen is initialized, and then the mouse is initialized. If the initialization of
* the mouse fails, the function will simply return. In an endless loop, the cursor will be read, then the
* cursor will be drawn. The drawing saves whatever is below where the cursor is being drawn in a temporary
* rectangle of space, then the rectangle is drawn for the cursor. If the cursor has been read 4 times since
* the last time the screen was updated, then screen will be updated again. Then, the saved rectangle of memory
* where the cursor is is redrawn to preserve what is under the cursor.
*/
void main(void) 
{
  gl_init(SCREEN_WIDTH, SCREEN_HEIGHT, GL_DOUBLEBUFFER);
  if (!mouse_init()) return;
  while (1) {
      readCursor();
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
