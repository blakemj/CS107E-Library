#include "console.h"
#include "gl.h"
#include "fb.h"
#include "printf.h"
#include "font.h"
#include <stdarg.h>
#include "strings.h"
#include "malloc.h"

static int numCols;
static int numRows;
static int lineNum = 0;
static int xCoord = 0;

void console_init(unsigned int nrows, unsigned int ncols)
{
    gl_init(ncols * font_get_width(), nrows * font_get_height(), GL_DOUBLEBUFFER);
    numCols = ncols;
    numRows = nrows;
}

void console_clear(void)
{
    gl_clear(0xff000000);
}

static void scroll() {
    unsigned char* temp = fb_get_draw_buffer() + fb_get_pitch() * gl_get_char_height();
    for (int i = 1; i < numRows; i++) {
        memcpy(fb_get_draw_buffer() + (i - 1) * fb_get_pitch() * gl_get_char_height(), temp, fb_get_pitch() * gl_get_char_height());
        temp = fb_get_draw_buffer() + (i + 1) * fb_get_pitch() * gl_get_char_height();
    }
    memset(fb_get_draw_buffer() + (numRows - 1) * fb_get_pitch() * gl_get_char_height(), 0, fb_get_pitch() * gl_get_char_height());
}

static void drawLine(char* str) {
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
        } else {
            gl_draw_char(xCoord * gl_get_char_width(), lineNum * font_get_height(), str[charIndex], 0xffff0000);
            xCoord++;
        } 
    }
}

int console_printf(const char *format, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 1024, format, args);
    drawLine(buf);
    gl_swap_buffer();
    va_end(args);
    return 0;
}
