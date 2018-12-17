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

/*
* This function will check all of the possible pixels in the anti-aliased line and will color those
* in that should be colored. The way this is done is by checking the distance between the line and 
* the center of the pixel (if the slope is > 1, then it will check horixontal distance, if < 1 then it
* will check vertical distance). This will then compute the difference (which should be less than one.
* If not, then it is not a pixel that is part of the line (as pixels more than a distance of one pixel
* away are considered off the line). If it is within a distance of one, then a shade of the line color
* will be colored in. This shade is some blend between what is currently in the pixel (aka the background)
* and what the line color is. The closer to the line, the closer to the line color it will be. This
* is done by using a sliding scale for each of the 3 components of the RGB values for the colors. This
* pixel is then drawn.
*/
static void poss_anti_aliased_line_pxs(double slope, int i, int j, int x1, int y1, color_t c) {
    double diff = 0;
    if (slope < 1 && slope > -1) {
        double yactual = slope * (i + 0.5);
        diff = yactual - j - 0.5;
    } else {
        double xactual = (j + 0.5) / slope;
        diff = xactual - i - 0.5;
    }
    if (diff < 0) diff = diff * -1;
    if (diff < 1) {
        color_t currColor = gl_read_pixel(i, j);
        char* curRGBA = (char*)&currColor;
        char* lineRGBA = (char*)&c;
        char lineBlue = *(curRGBA) - ((*(curRGBA) - *(lineRGBA)) * (1 - diff));
        char lineGreen = *(curRGBA + 1) - ((*(curRGBA + 1) - *(lineRGBA + 1)) * (1 - diff));
        char lineRed = *(curRGBA + 2) - ((*(curRGBA + 2) - *(lineRGBA + 2)) * (1 - diff));
        gl_draw_pixel(i + x1, j + y1, gl_color(lineRed, lineGreen, lineBlue));
    }
}

/*
* This function draws an anti-aliased line, always from right to left. Since it will check
* for all of the possible pixels in the line within the whole rectangle created by the line
* going between opposite corners, it will either move up or down the line depending on the
* slope. If the slope is 0 and is vertical (aka slope is undefined), then it will simply move
* up the line and draw the pixels in (since no anti-aliasing is needed). If the slope is 
* 0 and horizontal, then it will simply loop through all the x-values and draw the pixels (again
* since no anti-aliasing is needed). 
*/
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
    if (x2 == x1) slope = 0;
    if (slope > 0) {
        for (int j = 0; j < y2 - y1; j++) {
            for (int i = 0; i < x2 - x1; i++) {
                poss_anti_aliased_line_pxs(slope, i, j, x1, y1, c);
            }
        }
    } else if (slope < 0) {
        for (int j = 0; j < y2 - y1; j++) {
            for (int i = 0; i < x2 - x1; i++) {
                poss_anti_aliased_line_pxs(slope, i, j, x1, y1, c);
            }
        }
    } else {
        if (x2 == x1) {
            for (int j = 0; j <= y2 - y1; j++) {
                gl_draw_pixel(x1,j + y1, c);
            }
        } else {
            for (int i = 0; i <= x2 - x1; i++) {
                gl_draw_pixel(i + x1, y1, c);
            }
        }
    }
}

/*
* This function will draw the lines that fill in the triangle. The points are passed in, but the important
* points are (xmid, ymid) and (xother, yother)--which is one of the two diff points in the triangle. The funtion
* will create a triangle with a horizontal base, essentially splitting the whole triangle in two. This funtion 
* will then be called as these lines move closer and closer to the other point. This is because the lines are filled
* in one by one vertically. This is done by calculating the slope of each of the two lines (not including the base)
* and finding the x-values that are actually on the line. Since the line on the left will always use the pixel before
* the line, one is added to the x-value to make it the pixel after the line (and thus within the triangle (if it is on
* the line, then it will already have been colored in by the draw line function). This function gets called twice for 
* every triangle that does not have a flat base (one to go in the top half of the triangle, and one the bottom).
*/
static void drawFillLines(int ymid, int xmid, int ydiff1, int xdiff1, int ydiff2, int xdiff2, int yother, int xother, int j, color_t c) {
    double ptSlope = (double)(yother - ymid)/(double)(xother - xmid);
    if (xother == xmid) ptSlope = 0;
    double ptxactual = (j + ptSlope * xother - yother) / ptSlope;
    if (ptSlope == 0) ptxactual =  xmid;
    double oppSlope = (double)(ydiff1 - ydiff2)/(double)(xdiff1 - xdiff2);
    if (xdiff1 == xdiff2) oppSlope = 0;
    double oppxactual = (j + oppSlope * xdiff1 - ydiff1) / oppSlope;
    if (oppSlope == 0) oppxactual = xdiff1;
    if (ptxactual > oppxactual) { 
        oppxactual = oppxactual + 1;
    } else {
        ptxactual = ptxactual + 1;
    }
    gl_draw_line((int)ptxactual, j, (int)oppxactual, j, c);
}

/*
* This funtion starts by drawing lines to create anti-aliased boundaries. Next, lots of 
* checking is done to find out which of the three points is in the middle in the y/vertical
* direction. If one is found, then more checking is done to find which of the other two 
* points is above and which is below. It will then call the fill lines function to fill
* the triangle up to the point above the middle, and down to the point below the middle. 
* If there is not a middle point, that means that two points have the same y-value. So,
* a search will continue to figure out which points have the same y-value. Once one is
* found, the function will then check whether the third point is above or below the second
* If it is above, then it will iterate through the y-values upwards and draw the fill 
* lines. If it is below, then it will iterate downwards and draw the fill lines.
*/
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
