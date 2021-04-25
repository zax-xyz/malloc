#include "virtual_alloc.h"

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    virtual_sbrk(heapstart - virtual_sbrk(0));
    virtual_sbrk((1 << initial_size) + sizeof(block_t) + 2);

    uint8_t* prog_break = (uint8_t*) heapstart + (1 << initial_size);

    *(block_t*) prog_break = (block_t) {false, false, initial_size};
    prog_break += sizeof(block_t);

    *prog_break = initial_size;
    *(prog_break + 1) = min_size;
}

uint8_t log_2(uint32_t x) {
    uint8_t exp = 0;
    while (x > 1) {
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
    if (size == 0)
        return NULL;

    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(prog_break - 2);
    uint8_t min_size = *(prog_break - 1);

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
            memmove(block + 1 + diff,
                    block + 1,
                    prog_break - (uint8_t*) (block + 1));

            for (uint8_t i = diff; i > 0; i--) {
                block->size--;
                *(block + i) = (block_t) {false, true, block->size};
            }

            block->allocated = true;
            return block_ptr;
        }

        block_ptr += 1 << block->size;
    }

    return NULL;
}

bool should_merge_left(block_t* block) {
    return block->right && !(block - 1)->allocated
           && block->size == (block - 1)->size;
}

bool should_merge_right(block_t* block, uint8_t heap_size) {
    return block-> size != heap_size && !block->right && !(block + 1)->allocated
           && block->size == (block + 1)->size;
}

int virtual_free(void* heapstart, void* ptr) {
    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(prog_break - 2);

    block_t* blocks = (block_t*) ((uint8_t*) heapstart + (1 << heap_size));

    uint8_t* block_ptr = heapstart;
    for (block_t* block = blocks; (uint8_t*) block < prog_break - 2; block++) {
        if (block_ptr == ptr) {
            block->allocated = false;
            if (should_merge_left(block)) {
                while (should_merge_left(block)) {
                    (block - 1)->size++;
                    memmove(block,
                            block + 1,
                            prog_break - (uint8_t*) (block + 1));
                }
            } else if (should_merge_right(block, heap_size)) {
                while (should_merge_right(block, heap_size)) {
                    (block + 1)->size++;
                    memmove(block + 1,
                            block + 2,
                            prog_break - (uint8_t*) (block + 2));
                }
            }
        }

        block_ptr += 1 << block->size;
    }

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
