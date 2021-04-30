#ifndef VIRTUAL_ALLOC_H
#define VIRTUAL_ALLOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HEAP_START 8
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

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

int virtual_free(void* heapstart, void* ptr);

void* virtual_realloc(void* heapstart, void* ptr, uint32_t size);

void virtual_info(void* heapstart);

#endif
