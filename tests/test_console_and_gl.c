#include "assert.h"
#include "timer.h"
#include "fb.h"
#include "gl.h"
#include "console.h"
#include "printf.h"

/* Note that to use the console, one should only have to
 * call console_init. To use the graphics library, one
 * should only have to call gl_init. If your main() requires
 * more than this, then your code will not pass tests and
 * will likely have many points deducted. Our GL tests
 * will call gl_init then invoke operations; our console
 * tests will call console_init then invoke operations.
 * To guarantee that tests will pass, make sure to also
 * run tests for each component separately.
 */

#define _WIDTH 640
#define _HEIGHT 512

#define _NROWS 25
#define _NCOLS 20

static void test_console(void)
{
    console_init(_NROWS, _NCOLS);

    console_printf("Hi Avery.\nHow are you doing today?");

    console_printf("\nHELLO\r\n");

    timer_delay(3);
    console_printf("\f");
    console_printf("HI\r\n");
}

static void test_gl(void)
{
    gl_init(_WIDTH, _HEIGHT, GL_DOUBLEBUFFER);

    gl_clear(GL_BLUE); // Background should be purple.

//    gl_draw_rect(0, 0, _WIDTH, _HEIGHT, GL_RED);

//    gl_draw_rect(0, 0, _WIDTH + 30, _HEIGHT + 30, GL_RED);

    // Draw an amber pixel at an arbitrary spot.
//    gl_draw_pixel(_WIDTH/3, _HEIGHT/3, GL_AMBER);
//    assert(gl_read_pixel(_WIDTH/3, _HEIGHT/3) == GL_AMBER);

    // Basic rectangle should be blue in center of screen
//    gl_draw_rect(_WIDTH/2 - 20, _HEIGHT/2 - 20, 40, 40, GL_BLUE);

//    gl_draw_line(2, 2, 600, 250, GL_RED);

//    gl_draw_line(500, 500, 10, 2, GL_WHITE);

    gl_draw_triangle(250, 5, 10, 300, 150, 345, GL_GREEN);
    gl_draw_triangle(250, 5, 10, 5, 37, 200, GL_AMBER);
//    gl_draw_triangle(25, 5, 10, 300, 15, 345, GL_BLUE);
    gl_draw_triangle(250, 5, 250, 300, 255, 345, GL_RED);
//    gl_clear(gl_color(0xFF, 0, 0)); // Background should be purple.

    // Should write a single character
//    gl_draw_char(60, 10, 'A', GL_BLUE);

//    gl_draw_string(60, 10, "Hi Avery.", GL_BLUE);

    gl_swap_buffer();
}

void main(void)
{
//    test_console();
    test_gl();

    /* TODO: Add tests here to test your graphics library and console.
       For the framebuffer and graphics libraries, make sure to test
       single & double buffering and drawing/writing off the right or
       bottom edge of the frame buffer.
       For the console, make sure to test wrap-around and scrolling.
    */
}
