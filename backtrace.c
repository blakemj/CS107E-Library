#include "backtrace.h"
#include "printf.h"

static void find_name(unsigned int* pc, frame_t f[], int count) {
    pc = pc - 4;
    char* nameChars = (char*)pc;
    if (*(nameChars + 3) == 0xff) {
   // i    int nameSize = *nameChars + (*(nameChars + 1) * 16 * 16) + (*(nameChars + 2) * 16 * 16 * 16 * 16);
        int nameSize = *(unsigned int*)nameChars - 0xff000000;
        nameChars = nameChars - nameSize;
        f[count].name = nameChars;
    } else {
        f[count].name = "???";
    }
}

int backtrace(frame_t f[], int max_frames)
{
    void *cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));
    cur_fp = (void*)*((unsigned int*)cur_fp - 3);
    int count = 0;
    while ((int)cur_fp && (count < max_frames)) {
        unsigned int* pc = (unsigned int*)*(unsigned int*)cur_fp;
        f[count].resume_addr = (unsigned int)*((unsigned int*)cur_fp - 1);
        f[count].resume_offset = (unsigned int)(f[count].resume_addr - (unsigned int)(pc - 3));
        find_name(pc, f, count);
        cur_fp = (void*)*((unsigned int*)cur_fp - 3);
        count++;
    }
    return count;
}

void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}
