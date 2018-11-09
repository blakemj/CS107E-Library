#include "backtrace.h"
#include "printf.h"

/*
* This function moves from the program counter back to the header of
* the function being run to find out the number of characters. Once this
* is determined, the pointer will move back the number of characters 
* (including extra \0) and will the set the name equal to this pointer, which
* is effectively a string. If the name cannot be found, then the name is set
* to the string "???".
*/
static void find_name(unsigned int* pc, frame_t f[], int count) {
    pc = pc - 4;
    char* nameChars = (char*)pc;
    if (*(nameChars + 3) == 0xff) {
        int nameSize = *(unsigned int*)nameChars - 0xff000000;
        nameChars = nameChars - nameSize;
        f[count].name = nameChars;
    } else {
        f[count].name = "???";
    }
}

/*
* This function will take in an array of structs (frame_t), and a max number of frames in the
* array. The function will find the current frame pointer for the current function (not
* including the backtrace function) and will begin iterating through all of the caller frames
* to backtrace information. The information found in the stack helps find the address where
* the caller function will resume as well as the current program counter for the current
* function, which gives the offset number of addresses between the two. It will then find
* the name of the function and change the frame pointer to make back to the frame pointer from
* the previous function. The function returns the count of frames filled.
*/
int backtrace(frame_t f[], int max_frames)
{
    void *cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));
    void* next_fp = (void*)*((unsigned int*)cur_fp - 3);
    int count = 0;
    while ((int)next_fp && (count < max_frames)) {
        unsigned int* pc = (unsigned int*)*(unsigned int*)next_fp;
        f[count].resume_addr = (unsigned int)*((unsigned int*)cur_fp - 1);
        f[count].resume_offset = (unsigned int)(f[count].resume_addr - (unsigned int)(pc - 3));
        find_name(pc, f, count);
        cur_fp = next_fp;
        next_fp = (void*)*((unsigned int*)next_fp - 3);
        count++;
    }
    return count;
}

/*
* This function prints the information from the frames one by one
*/
void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

/*
* This function will print the entire backtrace from the function that it is initially called in.
*/
void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}
