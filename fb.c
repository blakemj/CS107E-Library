#include "mailbox.h"
#include "fb.h"
#include "strings.h"

// This prevents the GPU and CPU from caching mailbox messages
#define GPU_NOCACHE 0x40000000

typedef struct {
  unsigned int width;       // width of the display
  unsigned int height;      // height of the display
  unsigned int virtual_width;  // width of the virtual framebuffer
  unsigned int virtual_height; // height of the virtual framebuffer
  unsigned int pitch;       // number of bytes per row
  unsigned int depth;       // number of bits per pixel
  unsigned int x_offset;    // x of the upper left corner of the virtual fb
  unsigned int y_offset;    // y of the upper left corner of the virtual fb
  unsigned int framebuffer; // pointer to the start of the framebuffer
  unsigned int size;        // number of bytes in the framebuffer
} fb_config_t;

// fb is volatile because the GPU will write to it
static volatile fb_config_t fb __attribute__ ((aligned(16)));

/*
* This function initializes the frambuffer to the certain width, height, depth, and mode. depending on the mode, 
* a single or double framebuffer could be created.
*/
void fb_init(unsigned int width, unsigned int height, unsigned int depth, unsigned int mode)
{
  fb.width = width;
  fb.virtual_width = width;
  fb.height = height;
  if (mode == FB_SINGLEBUFFER) {
      fb.virtual_height = height;
  } else {
      fb.virtual_height = 2 * height;
  }
  fb.depth = depth * 8; // convert number of bytes to number of bits
  fb.x_offset = 0;
  fb.y_offset = 0;

  // the manual requires we to set these value to 0
  // the GPU will return new values
  fb.pitch = 0;
  fb.framebuffer = 0;
  fb.size = 0;

  mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned)&fb + GPU_NOCACHE);
  (void) mailbox_read(MAILBOX_FRAMEBUFFER);
}

/*
* This function will swap the framebuffer after copying all of the contents to the other buffer. This ensures
* that everything is saved.
*/
void fb_swap_buffer(void)
{
    if (fb.height == fb.virtual_height) return;
    if (fb.y_offset) {
        memcpy((char*)fb.framebuffer + fb.pitch * fb.height, (char*)fb.framebuffer, fb.pitch * fb.height); 
        fb.y_offset = 0;
    } else {
        memcpy((char*)fb.framebuffer, (char*)fb.framebuffer + fb.pitch * fb.height, fb.pitch * fb.height);
        fb.y_offset = fb.height;
    }
    mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned)&fb + GPU_NOCACHE);
    (void) mailbox_read(MAILBOX_FRAMEBUFFER);
}

/*
* This funtion will return whichever buffer is the caller buffer. This depends on the offset, so if the 
* GPU is reading the buffer with the offset, then the CPU should read the other and vice versa.
*/
unsigned char* fb_get_draw_buffer(void)
{
    if (fb.height == fb.virtual_height) return (unsigned char*)fb.framebuffer;
    if (fb.y_offset) {
        return (unsigned char*)fb.framebuffer;
    } else {
        return (unsigned char*)fb.framebuffer + fb.pitch * fb.height;
    }
    return 0;
}

// This function simply returns the width of the framebuffer
unsigned int fb_get_width(void)
{
    return fb.width;
}

// This function simply returns the height of the framebuffer
unsigned int fb_get_height(void)
{
    return fb.height;
}

// This funtion simply returns the depth of the framebuffer
unsigned int fb_get_depth(void)
{
    return fb.depth;
}

// This funtion simply returns the pitch of the framebuffer
unsigned int fb_get_pitch(void)
{
    return fb.pitch;
}

