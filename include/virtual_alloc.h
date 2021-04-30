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

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size);
void* virtual_malloc(void* heapstart, uint32_t size);
int virtual_free(void* heapstart, void* ptr);
void* virtual_realloc(void* heapstart, void* ptr, uint32_t size);
void virtual_info(void* heapstart);

#endif
