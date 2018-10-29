/*
 * File: malloc.c
 * --------------
 * This is the simple "bump" allocator from lecture.
 * An allocation request is serviced by tacking on the requested
 * space to the end of the heap thus far. 
 * It does not recycle memory (free is a no-op) so when all the
 * space set aside for the heap is consumed, it will not be able
 * to service any further requests.
 *
 * This code is given here just to show the very simplest of
 * approaches to dynamic allocation. You are to replace this code
 * with your own heap allocator implementation.
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

#define TOTAL_HEAP_SIZE 0x1000000 // 16 MB


/* Global variables for the bump allocator
 *
 * `heap_start` tracks where heap segment begins in memory.
 * It is initialized to point to the address at end of data segment.
 * It uses symbol __bss_end__ from memmap to locate end.
 * `heap_used` tracks the number of bytes allocated thus far.
 * The next available memory begins at heap_start + heap_used.
 * `heap_max` is total number of bytes set aside for heap segment.
 */
static void *heap_start = NULL;
static int heap_used = 0, heap_max = TOTAL_HEAP_SIZE;

static void reserve_space(void* start, size_t nbytes) {
    *(unsigned int*)((char*)start + 8 + nbytes) = *(unsigned int*)start - (nbytes + 8);
    *(unsigned int*)((char*)start + 8 + nbytes + 1) = 0;
    *(unsigned int*)start = nbytes;
    *((unsigned int*)start + 1) = 1;
}

void *malloc(size_t nbytes) 
{
    if (!heap_start) {
        heap_start = &__bss_end__;
        *(unsigned int*)heap_start = TOTAL_HEAP_SIZE - 8;
        *((unsigned int*)heap_start + 1) = 0;
    }
    if (heap_start != &__bss_end__) {
        heap_start = &__bss_end__;
    }
    nbytes = roundup(nbytes, 8);
    while (*(unsigned int*)heap_start < heap_max) {
        if ((*(unsigned int*)heap_start > nbytes + 8) && *((unsigned int*)heap_start + 1) == 0) {
            void* start = heap_start;
            reserve_space(start, nbytes);
            printf("it worked");
            return (void*)((unsigned int*)heap_start + 2);
        } else {
            heap_start = (void*)((char*)heap_start + *(unsigned int*)heap_start + 8);
        }
    }
    return NULL;
}

void free(void *ptr) 
{
    *((unsigned int*)ptr - 1) = 0;
    unsigned int next = *((unsigned int*)ptr - 2);
    unsigned int* nextHeader = (unsigned int*)((char*)ptr + 8 + next);
    while (*(nextHeader + 1) == 0) {
        *((unsigned int*)ptr - 2) = *((unsigned int*)ptr - 2) + *nextHeader;
        nextHeader = (unsigned int*)((char*)nextHeader + 8 + *nextHeader);
    }
}

void *realloc (void *old_ptr, size_t new_size)
{
    free(old_ptr);
    if (*((unsigned int*)old_ptr - 2) > (roundup(new_size, 8) + 8)) {
        reserve_space((void *)((unsigned int*) old_ptr - 2), roundup(new_size, 8));
        return old_ptr;
    }
    void *new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, old_ptr, new_size);
    return new_ptr;
}

void heap_dump () {
    // TODO: fill in your own code here.
}
