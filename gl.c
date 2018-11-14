#include "gl.h"
#include "font.h"
#include "strings.h"

#define DEPTH 4
#define ALPHA 0xff000000

/*
* This function is used to initialize the graphics library from the frame buffer
*/
void gl_init(unsigned int width, unsigned int height, unsigned int mode)
{
    fb_init(width, height, DEPTH, mode);
}

/*
* This function is called so that the the buffer can be switched and the graphics written can be displayed
*/
void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

/*
* This function returns the graphics width, which is the pitch returned by the GPU in the framebuffer
*/
unsigned int gl_get_width(void) 
{
    return fb_get_pitch() / 4;
}

/*
* This function returns the height of the graphics console, which is identical to that of the framebuffer
*/
unsigned int gl_get_height(void)
{
    return fb_get_height();
}

/*
* This funtion is used to set the color based on an RGB input. These are all shifted so that RGB are 
* concatenated onto an int set for full alpha, and this value is returned.
*/
color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    color_t color = ALPHA;
    color = color + (r << 16) + (g << 8) + b;
    return color;
}

/*
* This function sets every pixel on the graphics window to the color c
*/
void gl_clear(color_t c)
{
    for (int x = 0; x < gl_get_width(); x++) {
        for (int y = 0; y < gl_get_height(); y++) {
            gl_draw_pixel(x, y, c);
        }
    }
}

/*
* This function will draw a single pixel, assuming it is within the bounds of the framebuffer
*/
void gl_draw_pixel(int x, int y, color_t c)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    if (x < fb_get_width() && y < fb_get_height()) drawSpace[y][x] = c;
}

/*
* This function will return the color of a given pixel, assuming it is within the bounds of the framebuffer
*/
color_t gl_read_pixel(int x, int y)
{
    unsigned (*drawSpace)[gl_get_width()] = (unsigned (*)[gl_get_width()])fb_get_draw_buffer();
    if (x < fb_get_width() && y < fb_get_height()) return drawSpace[y][x];
    return ALPHA;
}

/*
* This function will draw a filled in rectangle of color c. The top left coordinates are passed in along with
* the width and height of the rectangle.
*/
void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int horiz = x; horiz < x + w; horiz++) {
        for (int vert = y; vert < y + h; vert++) {
            gl_draw_pixel(horiz, vert, c);
        }
    }
}

/*
* This function is used to draw characters to the console/graphics window. This will loop through all of the 
* pixels that are part of a character. If the pixel is set to a a code, then the pixel is filled in with the
* color c. If not, then the pixel is filled with whatever is already on the screen.
*/
void gl_draw_char(int x, int y, int ch, color_t c)
{
    unsigned char buf[gl_get_char_width() * gl_get_char_height()];
    font_get_char(ch, buf, gl_get_char_width() * gl_get_char_height());
    for (int pxheight = 0; pxheight < gl_get_char_height(); pxheight++) {
        for (int pxwidth = 0; pxwidth < gl_get_char_width(); pxwidth++) {
            if (buf[pxwidth + pxheight * gl_get_char_width()]) {
                gl_draw_pixel(x + pxwidth, y + pxheight, c);
            } else {
                gl_draw_pixel(x + pxwidth, y + pxheight, gl_read_pixel(x + pxwidth, y + pxheight));
            }
        }
    }
}

/*
* This function will take in a string, and will iterate through that string printing out all of the characters.
* this will stop if the string passes the bounds of the buffer
*/
void gl_draw_string(int x, int y, char* str, color_t c)
{
    int numChars = strlen(str);
    for (int charIndex = 0; charIndex < numChars; charIndex++) {
        gl_draw_char(x + charIndex * gl_get_char_width(), y, str[charIndex], c);
    }
}

/*
* This function returns the character height
*/
unsigned int gl_get_char_height(void)
{
    return font_get_height();
}

/*
* This function returns the character width
*/
unsigned int gl_get_char_width(void)
{
    return font_get_width();
}
