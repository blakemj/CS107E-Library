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

static void poss_anti_aliased_line_pxs(double slope, int i, int j, int x1, int y1, color_t c) {
    double yactual = slope * (i + 0.5) + y1;
    double diff = yactual - j - 0.5;
    if (diff < 0) diff = diff * -1;
    if (diff < 1) {
        color_t currColor = gl_read_pixel(i, j);
        char* curRGBA = (char*)&currColor;
        char* lineRGBA = (char*)&c;
        char lineBlue = *(curRGBA) - ((*(curRGBA) - *(lineRGBA)) * (1 - diff));
        char lineGreen = *(curRGBA + 1) - ((*(curRGBA + 1) - *(lineRGBA + 1)) * (1 - diff));
        char lineRed = *(curRGBA + 2) - ((*(curRGBA + 2) - *(lineRGBA + 2)) * (1 - diff));
        gl_draw_pixel(i + x1, j, gl_color(lineRed, lineGreen, lineBlue));
    }
}

void gl_draw_line(int x1, int y1, int x2, int y2, color_t c) {
    if (x2 < x1) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    double slope = (double)(y2 - y1) / (double)(x2 - x1);
    for (int i = 0; i < x2 - x1; i++) {
        if (slope > 0) {
            for (int j = y1; j < y2; j++) {
                poss_anti_aliased_line_pxs(slope, i, j, x1, y1, c);
            }
        } else if (slope < 0) {
            for (int j = y1; j > y2; j--) {
                poss_anti_aliased_line_pxs(slope, i, j, x1, y1, c);
            }
        } else {
            gl_draw_pixel(i + x1, y1, c);
        }
    }

}

static void drawFillLines(int ymid, int xmid, int ydiff1, int xdiff1, int ydiff2, int xdiff2, int yother, int xother, int j, color_t c) {
    double ptSlope = (double)(yother - ymid)/(double)(xother - xmid);
    double ptxactual = (j + ptSlope * xother - yother) / ptSlope;
    double oppSlope = (double)(ydiff1 - ydiff2)/(double)(xdiff1 - xdiff2);
    double oppxactual = (j + oppSlope * xdiff1 - ydiff1) / oppSlope;
    if (ptxactual > oppxactual) { 
        oppxactual = oppxactual + 1;
    } else {
        ptxactual = ptxactual + 1;
    }
    gl_draw_line((int)ptxactual, j, (int)oppxactual, j, c);
}

void gl_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t c) {
    gl_draw_line(x1, y1, x2, y2, c);
    gl_draw_line(x2, y2, x3, y3, c);
    gl_draw_line(x3, y3, x1, y1, c);
    if ((y1 < y2 && y1 > y3) || (y1 > y2 && y1 < y3)) {
        if (y2 < y3) {
            for (int j = y1; j < y3; j++) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y3, x3, j, c);
            }
            for (int j = y1; j > y2; j--) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y2, x2, j, c);
            }
        } else {
            for (int j = y1; j > y3; j--) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y3, x3, j, c);
            }
            for (int j = y1; j < y1; j++) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y2, x2, j, c);
            }
        }
    } else if ((y2 < y1 && y2 > y3) || (y2 > y1 && y2 < y3)) {
        if (y1 < y3) {
            for (int j = y2; j < y3; j++) {
                drawFillLines(y2, x2, y1, x1, y3, x3, y3, x3, j, c);
            }
            for (int j = y2; j > y1; j--) {
                drawFillLines(y2, x2, y1, x1, y3, x3, y1, x1, j, c);
            }
        } else {
            for (int j = y2; j > y3; j--) {
                drawFillLines(y2, x2, y1, x1, y3, x3, y3, x3, j, c);
            }   
            for (int j = y2; j < y1; j++) {
                drawFillLines(y2, x2, y1, x1, y3, x3, y1, x1, j, c);
            }
        }
    } else if ((y3 < y1 && y3 > y2) || (y3 > y1 && y3 < y2)) {
        if (y1 < y2) {
            for (int j = y3; j < y2; j++) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y2, x2, j, c);
            }
            for (int j = y3; j > y1; j--) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y1, x1, j, c);
            }
        } else {
            for (int j = y3; j > y2; j--) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y2, x2, j, c);
            }
            for (int j = y3; j < y1; j++) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y1, x1, j, c);
            }
        }
    } else if (y1 == y2) {
        if (y1 < y3) {
            for (int j = y1; j < y3; j++) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y3, x3, j, c);
            }
        } else {
            for (int j = y1; j > y3; j--) {
                drawFillLines(y1, x1, y2, x2, y3, x3, y3, x3, j, c);
            }
       }
    } else if (y2 == y3) {
        if (y2 < y1) {
            for (int j = y2; j < y1; j++) {
                drawFillLines(y2, x2, y3, x3, y1, x1, y1, x1, j, c);
            } 
        } else {
            for (int j = y2; j > y1; j--) {
                drawFillLines(y2, x2, y3, x3, y1, x1, y1, x1, j, c);
            }
       }
    } else if (y3 == y1) {
        if (y3 < y2) {
            for (int j = y3; j < y2; j++) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y2, x2, j, c);
            } 
        } else {
            for (int j = y3; j > y2; j--) {
                drawFillLines(y3, x3, y1, x1, y2, x2, y2, x2, j, c);
            }
       }
    }
}
