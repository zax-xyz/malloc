#ifndef HELPERS_H
#define HELPERS_H

#include "virtual_alloc.h"

uint8_t log_2(uint32_t x);
block_t* smallest_block(void* heapstart, uint8_t min_size, uint8_t** ptr);

int merge_blocks(void* heapstart, block_t* block, uint8_t* block_ptr);
bool should_merge_left(block_t* block);
bool should_merge_right(block_t* block, uint8_t heap_size);
bool is_right(uint8_t size, uint8_t* heapstart, uint8_t* block_ptr);

void shift(block_t* block, uint8_t* prog_break, int16_t offset);
block_t* get_block_info(void* heapstart, void* ptr);


#endif