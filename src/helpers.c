#include "virtual_alloc.h"

uint8_t log_2(uint32_t x) {
    uint8_t exp = 0;
    uint32_t counter = 1;
    while (counter < x) {
        exp++;
        counter <<= 1;
    }

    return exp;
}

block_t* smallest_block(void* heapstart, uint8_t min_size, uint8_t** ptr) {
    uint8_t lowest_size = UINT8_MAX;
    block_t* smallest_block = NULL;
    uint8_t* smallest_block_ptr = NULL;

    uint8_t heap_size = * (uint8_t*) heapstart;
    uint8_t* heap = (uint8_t*) heapstart + 2;

    block_t* info_start = (block_t*) heap + (1 << heap_size);

    block_t* block = info_start;

    for (; *ptr < (uint8_t*) info_start; *ptr += 1 << block->size, block++) {
        if (!block->allocated && block->size >= min_size
                && block->size < lowest_size) {
            lowest_size = block->size;
            smallest_block = block;
            smallest_block_ptr = *ptr;
        }
    }

    *ptr = smallest_block_ptr;

    return smallest_block;
}

int merge_blocks(void* heapstart, block_t* block, uint8_t* block_ptr) {
    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(uint8_t*) heapstart;

    while (1) {
        bool right = is_right(block->size, (uint8_t*) heapstart, block_ptr);

        if (right && should_merge_left(block)) {
            (block - 1)->size++;
            shift(block + 1, prog_break, -1);
            block--;
            block_ptr -= 1 << (block->size - 1);
        } else if (!right && should_merge_right(block, heap_size)) {
            (block + 1)->size++;
            shift(block + 1, prog_break, -1);
        } else {
            break;
        }

        // shrink heap
        if (virtual_sbrk(-1) == (void*) -1)
            return 1;

        prog_break -= 1;
    }

    return 0;
}

bool should_merge_left(block_t* block) {
    block_t* left = block - 1;
    return !left->allocated && block->size == left->size;
}

bool should_merge_right(block_t* block, uint8_t heap_size) {
    block_t* right = block + 1;
    return block->size != heap_size && !right->allocated
           && block->size == right->size;
}

bool is_right(uint8_t size, uint8_t* heapstart, uint8_t* block_ptr) {
    uint8_t heap_size = *heapstart;
    uint8_t* start = heapstart + 2;

    uint8_t* end = start + (1 << heap_size);

    uint8_t* mid;

    while (1) {
        if (block_ptr + (1 << size) == end)
            return true;

        if (block_ptr == start)
            return false;

        mid = start + (end - start) / 2;

        if (block_ptr >= mid) {
            start = mid;
        } else {
            end = mid;
        }
    }
}

void shift(block_t* block, uint8_t* prog_break, int16_t offset) {
    memmove(block + offset, block, prog_break - (uint8_t*) block);
}

block_t* get_block_info(void* heapstart, void* ptr) {
    uint8_t heap_size = *(uint8_t*) heapstart;

    block_t* blocks = (block_t*) heapstart + 2 + (1 << heap_size);
    uint8_t* block_ptr = heapstart + 2;

    for (block_t* block = blocks; block_ptr < (uint8_t*) blocks; block++) {
        if (block_ptr == ptr)
            return block;

        block_ptr += 1 << block->size;
    }

    return NULL;
}