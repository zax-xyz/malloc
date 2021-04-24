#include "virtual_alloc.h"
#include <stdbool.h>
#include <string.h>

#define HEAP_START 8
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct {
    bool allocated;
    uint8_t size;
} heap_block;

extern void* virtual_sbrk(uint32_t increment);

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    uint8_t* prog_break = virtual_sbrk(sizeof(heap_block) + 2 * sizeof(uint8_t));

    * (heap_block*) prog_break = (heap_block) { false, initial_size };
    prog_break += sizeof(heap_block);

    *prog_break = initial_size;
    *(prog_break + 1) = min_size;
}

uint8_t log_2(uint32_t x) {
    uint8_t exp = 1;
    while (x > 2) {
        exp++;
        x >>= 1;
    }

    return exp;
}

uint8_t smallest_block(heap_block* blocks, void* prog_break, uint8_t min_size) {
    uint8_t lowest_size = UINT8_MAX;

    for (heap_block* block = blocks; (void*) block < prog_break; block++)
        if (!block->allocated && block->size >= min_size)
            lowest_size = MIN(lowest_size, block->size);

    return lowest_size;
}

void* virtual_malloc(void* heapstart, uint32_t size) {
    if (size == 0)
        return NULL;
    
    void* prog_break = virtual_sbrk(0);
    uint8_t heap_size = * (uint8_t*) (prog_break - 2);
    uint8_t min_size = * (uint8_t*) (prog_break - 1);

    if (size > 1 << heap_size)
        return NULL;

    uint8_t needed_size = MAX(min_size, log_2(size));

    heap_block* blocks = (heap_block*) ((uint8_t*) heapstart + heap_size);

    uint8_t lowest_size = smallest_block(blocks, prog_break, needed_size);
    if (lowest_size == UINT8_MAX)
        return NULL;

    uint8_t diff = lowest_size - needed_size;
    if (diff)
        virtual_sbrk(sizeof(heap_block) * diff);

    uint8_t* block_ptr = heapstart;
    for (heap_block* block = blocks; (void*) block < prog_break; block++) {
        if (block->size == lowest_size) {
            if (diff) {
                memmove(
                    block + 1,
                    block + 1 + diff,
                    (uint8_t*) prog_break - (uint8_t*) block
                );
            }

            for (uint8_t i = diff; i > 0; i--) {
                block->size--;
                *(block + i) = (heap_block) { false, block->size };
            }

            block->allocated = true;
            return block_ptr;
        }

        block_ptr += 1 << block->size;
    }

    return NULL;
}

int virtual_free(void* heapstart, void* ptr) {
    // Your code here
    return 1;
}

void * virtual_realloc(void* heapstart, void* ptr, uint32_t size) {
    // Your code here
    return NULL;
}

void virtual_info(void* heapstart) {
    // Your code here
}
