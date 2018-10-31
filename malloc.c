/*
 * File: malloc.c
 */

#include "malloc.h"
#include <stddef.h> // for NULL
#include "printf.h"
#include "strings.h"

extern int __bss_end__;

// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two -- why?
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

//This defines the size of the heap.
#define TOTAL_HEAP_SIZE 0x1000000 // 16 MB

//The global variable for heap_start maintains the placement of the pointer in the heap.
static void *heap_start = NULL;
//This maintains the maximum heap size.
static int heap_max = TOTAL_HEAP_SIZE;

/*
* This funciton is used to reserve spece within the heap. This is done by setting the 
* header of the space to include in the first 4 bytes the size of the block in bytes
* (not including the headers), and in the second, a 1 which represents "reserved." 
* Then, it will move beyond the size of bytes that the user had asked for (the size
* of bytes from the previous header), and will create a new header for the next 
* block of the heap. This header includes the amount of memory available after it, 
* and a 0 to represent that the space is "available."
*/
static void reserve_space(void* start, size_t nbytes) {
    *(unsigned int*)((char*)start + 8 + nbytes) = *(unsigned int*)start - (nbytes + 8);
    *(unsigned int*)((char*)start + 8 + nbytes + 4) = 0;
    *(unsigned int*)start = nbytes;
    *((unsigned int*)start + 1) = 1;
}

/*
* This function is called to allocate a block of memory for the number of bytes
* requested rounded to the nearest multiple of 8 (for alignment purposes). On 
* the first call, the heap is initialized. Then, on every other call, the 
* allocation will search for free space starting from the beginning header on
* the heap. If the spot pointed to has enough space for the memory (as well as
* the addition of another header plus 8 (smallest space that can be malloc-ed),
* and the spot is available, then malloc will reserve that space and return
* a pointer. If not, then it will move on to the next block and repeat the loop
* until it reaches the end of the heap. If this happens, or if the requested
* space is zero, then the functon will return a NULL pointer to the user.
*/
void *malloc(size_t nbytes) 
{
    // This initializes the start of the heap to start at the end of the bss
    // block, but then also create an initial header.
    if (!heap_start) {
        heap_start = &__bss_end__;
        *(unsigned int*)heap_start = (heap_max - 8);
        *((unsigned int*)heap_start + 1) = 0;
    }

    if (heap_start != &__bss_end__) {
        heap_start = &__bss_end__;
    }
    nbytes = roundup(nbytes, 8);
    while (*(unsigned int*)heap_start <= TOTAL_HEAP_SIZE - 8 && nbytes != 0) {
        if ((*(unsigned int*)heap_start > nbytes + 8) && *((unsigned int*)heap_start + 1) == 0) {
            void* start = heap_start;
            reserve_space(start, nbytes);
            return (void*)((unsigned int*)heap_start + 2);
        } else {
            heap_start = (void*)((char*)heap_start + *(unsigned int*)heap_start + 8);
        }
    }
    return NULL;
}

/*
* This funtion is used to take in a pointer to a spot on the stack, and to free the
* space that has been previously malloc-ed. First, it will move backwards and set the
* header to indicate that the space is "available." It will then use the header to 
* figure out where the next memory block is. It will travel down the heap and free 
* all available blocks, coalescing them all into one large freed block of memory.
* This minimizes the fragmentation of memory.
*/
void free(void *ptr) 
{
    *((unsigned int*)ptr - 1) = 0;
    unsigned int next = *((unsigned int*)ptr - 2);
    unsigned int* nextHeader = (unsigned int*)((char*)ptr + next);
    while (*(nextHeader + 1) == 0) {
        *((unsigned int*)ptr - 2) = *((unsigned int*)ptr - 2) + *nextHeader + 8;
        nextHeader = (unsigned int*)((char*)nextHeader + 8 + *nextHeader);
    }
}

/*
* This function will reallocate space for a given pointer that has previously been
* malloc-ed to a new size. The function frees the space, and then attempts to first
* put the memory back into the space it was in (but at a larger size). (By freeing 
* first, we ensure that we coalesce all free memory). If this is not possible, it 
* will then leave the block freed, and will then re-malloc the block to a new pointer.
* If this pointer does not return NULL (i.e. there is enough space/size != 0), then 
* all of the previous memory from the old block is copied into the new block, and this
* new block is then returned.
*/
void *realloc (void *old_ptr, size_t new_size)
{
    free(old_ptr);
    if (*((unsigned int*)old_ptr - 2) > (roundup(new_size, 8) + 8) && old_ptr && new_size) {
        reserve_space((void *)((unsigned int*) old_ptr - 2), roundup(new_size, 8));
        return old_ptr;
    }
    void *new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, old_ptr, new_size);
    return new_ptr;
}

/*
* This function will start at the beginning of the heap, and will print out all headers for
* blocks of memory created in the heap, and their status as "reserved" or "available."
*/
void heap_dump () {
    void* start = &__bss_end__;
    while ((unsigned int*)start < (unsigned int*)&__bss_end__ + (TOTAL_HEAP_SIZE / 4)) {
        printf("%p -> %x\n", start, *(unsigned int*)start);
        printf("%p -> %x\n\n", (unsigned int*)start + 1, *((unsigned int*)start + 1));
        start = (unsigned int*)start + (*(unsigned int*)start / 4 + 2);
    }
}
