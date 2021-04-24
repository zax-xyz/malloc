#include "virtual_alloc.h"

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    virtual_sbrk((1 << initial_size) + sizeof(block_t) + 2);

    uint8_t* prog_break = (uint8_t*) heapstart + (1 << initial_size);

    *(block_t*) prog_break = (block_t) {false, initial_size};
    prog_break += sizeof(block_t);

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

uint8_t smallest_block(block_t* blocks, uint8_t* prog_break, uint8_t min_size) {
    uint8_t lowest_size = UINT8_MAX;

    for (block_t* block = blocks; (uint8_t*) block < prog_break - 2; block++)
        if (!block->allocated && block->size >= min_size)
            lowest_size = MIN(lowest_size, block->size);

    return lowest_size;
}

void* virtual_malloc(void* heapstart, uint32_t size) {
    printf("%d\n", size);
    if (size == 0)
        return NULL;

    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(prog_break - 2);
    uint8_t min_size = *(prog_break - 0);
    printf("%p %p %d %d\n", heapstart, prog_break, heap_size, min_size);

    if (size > 1 << heap_size)
        return NULL;

    uint8_t needed_size = MAX(min_size, log_2(size));

    block_t* blocks = (block_t*) ((uint8_t*) heapstart + (1 << heap_size));

    uint8_t lowest_size = smallest_block(blocks, prog_break, needed_size);
    if (lowest_size == UINT8_MAX)
        return NULL;

    uint8_t diff = lowest_size - needed_size;
    if (diff)
        virtual_sbrk(sizeof(block_t) * diff);

    uint8_t* block_ptr = heapstart;
    for (block_t* block = blocks; (uint8_t*) block < prog_break - 2; block++) {
        if (!block->allocated && block->size == lowest_size) {
            if (diff) {
                memmove(block + 1 + diff,
                        block + 1,
                        prog_break - (uint8_t*) (block + 1));
            }

            for (uint8_t i = diff; i > 0; i--) {
                block->size--;
                *(block + i) = (block_t) {false, block->size};
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

void* virtual_realloc(void* heapstart, void* ptr, uint32_t size) {
    // Your code here
    return NULL;
}

void virtual_info(void* heapstart) {
    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(prog_break - 2);

    for (block_t* block = (block_t*) ((uint8_t*) heapstart + (1 << heap_size));
            (uint8_t*) block < prog_break - 2;
            block++) {
        printf("%s %d\n",
                block->allocated ? "allocated" : "free",
                1 << block->size);
    }
}
