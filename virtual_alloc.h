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

#define ALLOCATED(BLOCK) ((BLOCK) & 1)
#define SET_ALLOCATED(BLOCK, ALLOC) (BLOCK) = ((BLOCK) & ~1) | (ALLOC)

#define SIZE(BLOCK) ((BLOCK) >> 1)
#define INC_SIZE(BLOCK) BLOCK = ((SIZE(BLOCK) + 1) << 1) + ALLOCATED(BLOCK)
#define DEC_SIZE(BLOCK) BLOCK = ((SIZE(BLOCK) - 1) << 1) + ALLOCATED(BLOCK)

uint8_t log_2(uint32_t x);

extern void* virtual_sbrk(int32_t increment);

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size);

void* virtual_malloc(void* heapstart, uint32_t size);

int virtual_free(void* heapstart, void* ptr);

void* virtual_realloc(void* heapstart, void* ptr, uint32_t size);

void virtual_info(void* heapstart);

#endif
