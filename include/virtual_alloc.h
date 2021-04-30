#ifndef VIRTUAL_ALLOC_H
#define VIRTUAL_ALLOC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

// A struct to hold information about a block using bitfields. Should be a
// single byte.
typedef struct {
    bool allocated : 1;
    uint8_t size: 7;
} block_t;

#include "helpers.h"

extern void* virtual_sbrk(int32_t increment);

/*
 * Initialises the virtual heap with size 2^initial_size bytes, with minimum
 * block size 2^min_size. Resets the heap to an empty size before allocating
 * enough space for the heap and for information about the heap.
 */
void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size);

/*
 * Emulates malloc on the virtual heap. Follows the buddy allocation algorithm.
 * Allocates the block in the leftmost unallocated position that is sufficiently
 * large by splitting until reaching the desired size. Allocates blocks in sizes
 * of powers of 2. If allocation is not possible, returns NULL.
 */
void* virtual_malloc(void* heapstart, uint32_t size);

/*
 * Emulates free on the virtual heap according to the buddy algorithm.
 * Unallocates a block pointed to by ptr and merges it with its buddy if the
 * buddy is also unallocated. Repeats the process until no longer possible.
 * Returns 0 if successful, 1 if not.
 */
int virtual_free(void* heapstart, void* ptr);

/*
 * Emulates realloc on the virtual heap using the buddy allocation algorithm.
 * Attempts to resize a block to a specified size, moving it if necessary.
 * If the specified size is lower than the original size, truncates the data.
 * If the block is unable to be reallocated, the heap is left unchanged and NULL
 * is returned. Otherwise, a pointer to the new block is returned. If ptr is
 * NULL, 
 */
void* virtual_realloc(void* heapstart, void* ptr, uint32_t size);

/*
 * Prints information about each block in the heap, from left (smallest address)
 * to right. For each block, displays whether it is allocated or free, and its
 * size. The size is given as an exponent of 2. I.e., 2^size == "actual" size
 */
void virtual_info(void* heapstart);

#endif
