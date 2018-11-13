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
    return fb_get_pitch() / 4;
}

unsigned int gl_get_height(void)
{
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
}

void gl_draw_pixel(int x, int y, color_t c)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    if (x < fb_get_width() && y < fb_get_height()) drawSpace[y][x] = c;
}

color_t gl_read_pixel(int x, int y)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    return drawSpace[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int hline = x; hline < x + w; hline++) {
        gl_draw_pixel(hline, y, c);
        gl_draw_pixel(hline, y + h - 1, c);
    }
    for (int vline = y + 1; vline < y + h - 1; vline++) {
        gl_draw_pixel(x, vline, c);
        gl_draw_pixel(x + w - 1, vline, c);
    }
}

void gl_draw_char(int x, int y, int ch, color_t c)
{
    unsigned char buf[gl_get_char_width() * gl_get_char_height()];
    font_get_char(ch, buf, gl_get_char_width() * gl_get_char_height());
    for (int pxheight = 0; pxheight < gl_get_char_height(); pxheight++) {
        for (int pxwidth = 0; pxwidth < gl_get_char_width(); pxwidth++) {
            if (buf[pxwidth + pxheight * gl_get_char_width()]) {
                gl_draw_pixel(x + pxwidth, y + pxheight, c);
            } else {
                gl_draw_pixel(x + pxwidth, y + pxheight, 0xff000000); // gl_read_pixel(x + pxwidth, y + pxheight));
            }
        }
    }
}

void gl_draw_string(int x, int y, char* str, color_t c)
{
    int numChars = strlen(str);
    for (int charIndex = 0; charIndex < numChars; charIndex++) {
        gl_draw_char(x + charIndex * gl_get_char_width(), y, str[charIndex], c);
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_width();
}
