#include "console.h"
#include "gl.h"
#include "fb.h"
#include "printf.h"
#include "font.h"
#include <stdarg.h>
#include "strings.h"
#include "malloc.h"

//For the extension, you can actually set the background color, and even the scroll with go with it
#define BACK_COLOR GL_RED
#define CHAR_COLOR GL_BLUE
#define INIT_BUFFER_LENGTH 1024

static int numCols;
static int numRows;
//These variables keep track of the location of the cursor
static int lineNum = 0;
static int xCoord = 0;

/*
* This function initializes the console. It sets global variables so that other functions can axess the
* number of rows and columns for scrolling and wrapping.
*/
void console_init(unsigned int nrows, unsigned int ncols)
{
    gl_init(ncols * font_get_width(), nrows * font_get_height(), GL_DOUBLEBUFFER);
    console_clear();
    numCols = ncols;
    numRows = nrows;
}

/*
* This function will simply clear the console back to the background color
*/
void console_clear(void)
{
    gl_clear(BACK_COLOR);
}

/*
* This function will scroll the screen upwards when a newline is written past the screen. The top line on the screen
* disappears and all lines move up one. Then the bottom line becomes the line being written into.
*/
static void scroll() {
    unsigned char* temp = fb_get_draw_buffer() + fb_get_pitch() * gl_get_char_height();
    for (int i = 1; i < numRows; i++) {
        memcpy(fb_get_draw_buffer() + (i - 1) * fb_get_pitch() * gl_get_char_height(), temp, fb_get_pitch() * gl_get_char_height());
        temp = fb_get_draw_buffer() + (i + 1) * fb_get_pitch() * gl_get_char_height();
    }
    gl_draw_rect(0, lineNum * gl_get_char_height(), numCols * gl_get_char_width(), gl_get_char_height(), BACK_COLOR);
}

/*
* This function is used to draw a line into the screen. First, if the line reaches the end of the console, it will wrap
* around to the next line, even if a newline wasn't called. Next, if the number of rows is filled, then it will scroll
* the screen. Next, it will check for special characters. If newline is called, then the current line number is increased.
* If the backspace character is called, then the cursor internally moves back one column. If the '\f' character is called,
* then the screen is cleared and everything is reset back to initial conditions. If the space character is called, then it
* will draw a rectangle the color of the background. This ensures that when backspacing, the previous character is covered.
* Otherwise, the character is simply drawn from the graphics library.
*/
static int drawLine(char* str) {
    int numCharsDrawn = 0;
    for (int charIndex = 0; charIndex < strlen(str); charIndex++) {
        if (xCoord != 0 && xCoord % numCols == 0) {
            lineNum++;
            xCoord = 0;
        }
        if (lineNum >= numRows) {
            lineNum--;
            scroll();
        }
        if (str[charIndex] == '\n') {
            lineNum++;
            xCoord = 0;
        } else if (str[charIndex] == '\b') {
            xCoord--;
        } else if (str[charIndex] == '\f') {
            console_clear();
            lineNum = 0;
            xCoord = 0;
        } else if (str[charIndex] == ' ' || str[charIndex] == '\r') {
            gl_draw_rect(xCoord * gl_get_char_width(), lineNum * font_get_height(), gl_get_char_width(), font_get_height(), BACK_COLOR);
            xCoord++;
            numCharsDrawn++;
        } else {
            numCharsDrawn++;
            gl_draw_char(xCoord * gl_get_char_width(), lineNum * font_get_height(), str[charIndex], CHAR_COLOR);
            xCoord++;
        } 
    }
    return numCharsDrawn;
}

/*
* This function is used to call printf to the console. This function also swaps the buffer so that the graphics window is updated.
*/
int console_printf(const char *format, ...)
{
    char buf[INIT_BUFFER_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, INIT_BUFFER_LENGTH, format, args);
    int numCharsDrawn = drawLine(buf);
    gl_swap_buffer();
    va_end(args);
    return numCharsDrawn;
}

