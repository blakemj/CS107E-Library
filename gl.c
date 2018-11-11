#include "gl.h"
#include "font.h"
#include "strings.h"

#define DEPTH 4

void gl_init(unsigned int width, unsigned int height, unsigned int mode)
{
    fb_init(width, height, DEPTH, mode);
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

unsigned int gl_get_width(void) 
{
    // TODO: implement this function
    return fb_get_pitch() / 4;
}

unsigned int gl_get_height(void)
{
    // TODO: implement this function
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    color_t color = 0xff000000;
    color = color + (r << 16) + (g << 8) + b;
    return color;
}

void gl_clear(color_t c)
{
    for (int i = 0; i < gl_get_width(); i++) {
        for (int j = 0; j < gl_get_height(); j++) {
            gl_draw_pixel(i, j, c);
        }
    }
//    gl_swap_buffer();
}

void gl_draw_pixel(int x, int y, color_t c)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    drawSpace[y][x] = c;
}

color_t gl_read_pixel(int x, int y)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    return drawSpace[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    if (x + w > gl_get_width()) w = gl_get_width() - x;
    if (y + h > gl_get_height()) h = gl_get_height() - y; 
    for (int i = x; i < x + w; i++) {
        gl_draw_pixel(i, y, c);
        gl_draw_pixel(i, y + h - 1, c);
    }
    for (int j = y + 1; j < y + h - 1; j++) {
        gl_draw_pixel(x, j, c);
        gl_draw_pixel(x + w - 1, j, c);
    }
//    gl_swap_buffer();
}

void gl_draw_char(int x, int y, int ch, color_t c)
{
    // TODO: implement this function
    unsigned char buf[gl_get_char_width() * gl_get_char_height()];
    font_get_char(ch, buf, gl_get_char_width() * gl_get_char_height());
    for (int i = 0; i < gl_get_char_height(); i++) {
        if (y + i > gl_get_height()) return;
        for (int j = 0; j < gl_get_char_width(); j++) {
            if (x + j > gl_get_width()) return;
            if (buf[j + i * gl_get_char_width()]) {
                gl_draw_pixel(x + j, y + i, c);
            } else {
                gl_draw_pixel(x + j, y + i, 0xff000000);
            }
        }
    }
}

void gl_draw_string(int x, int y, char* str, color_t c)
{
    // TODO: implement this function
    int numChars = strlen(str);
    for (int i = 0; i < numChars; i++) {
        gl_draw_char(x + i * gl_get_char_width(), y, str[i], c);
    }
}

unsigned int gl_get_char_height(void)
{
    // TODO: implement this function
    return font_get_height();
}

unsigned int gl_get_char_width(void)
{
    // TODO: implement this function
    return font_get_width();
}

