#include "virtual_alloc.h"

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
#ifdef DEBUG
    printf("INIT %d %d\n", initial_size, min_size);
#endif

    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    virtual_sbrk(heapstart - virtual_sbrk(0));  // reset heap
    virtual_sbrk((1 << initial_size) + 1 + 2);

    uint8_t* prog_break = (uint8_t*) heapstart + (1 << initial_size);

    *(prog_break + 2) = initial_size << 1;

    *(uint8_t*) heapstart = initial_size;
    *((uint8_t*) heapstart + 1) = min_size;
}

void* virtual_malloc(void* heapstart, uint32_t size) {
#ifdef DEBUG
    printf("ALLOC %d\n", size);
#endif

    if (size == 0)
        return NULL;

    uint8_t heap_size = *(uint8_t*) heapstart;
    uint8_t min_size = *((uint8_t*) heapstart + 1);

    if (size > 1 << heap_size)
        return NULL;

    uint8_t needed_size = MAX(min_size, log_2(size));

    uint8_t* ptr = (uint8_t*) heapstart + 2;
    block_t* block = smallest_block(heapstart, needed_size, &ptr);
    if (block == NULL)
        return NULL;

    uint8_t diff = block->size - needed_size;
    virtual_sbrk(diff);

    shift(block + 1, virtual_sbrk(0), diff);

    for (uint8_t i = diff; i > 0; i--) {
        block->size--;
        *(block + i) = (block_t) {false, block->size};
    }

    block->allocated = true;

    return ptr;
}

int virtual_free(void* heapstart, void* ptr) {
#ifdef DEBUG
    printf("FREE %lu\n", (size_t)((uint8_t*) ptr - (uint8_t*) heapstart) - 2);
#endif

    block_t* block = get_block_info(heapstart, ptr);
    if (block == NULL)
        return 1;

    if (!block->allocated)
        return 1;

    block->allocated = false;
    block = merge_blocks(heapstart, block, ptr);

    return 0;
}

void* virtual_realloc(void* heapstart, void* ptr, uint32_t size) {
#ifdef DEBUG
    printf("REALLOC %lu %u\n", (uint8_t*) ptr - (uint8_t*) heapstart - 2, size);
#endif

    if (size == 0) {
        virtual_free(heapstart, ptr);
        return NULL;
    }

    if (ptr == 0)
        return virtual_malloc(heapstart, size);

    uint8_t* prog_break = virtual_sbrk(0);
    size_t heap_size = 1 << *(uint8_t*) heapstart;
    uint8_t* heap = (uint8_t*) heapstart + 2;

    if (size > heap_size)
        return NULL;
    
    block_t* block = get_block_info(heapstart, ptr);
    uint32_t og_size = 1 << block->size;

    virtual_sbrk(og_size + heap_size);
    memmove(prog_break, ptr, og_size);
    memmove(prog_break + og_size, heap, heap_size);

    virtual_free(heapstart, ptr);
    void* new_block = virtual_malloc(heapstart, size);

    uint8_t* new_prog_break = virtual_sbrk(0);

    if (new_block == NULL) {
        memmove(heap, new_prog_break - heap_size, heap_size);
        virtual_sbrk(-heap_size - og_size);
        return NULL;
    }

    memmove(new_block,
            new_prog_break - heap_size - og_size,
            MIN(og_size, size));
    virtual_sbrk(-heap_size - og_size);

    return new_block;
}

void virtual_info(void* heapstart) {
#ifdef DEBUG
    printf("INFO\n");
#endif

    uint8_t* prog_break = virtual_sbrk(0);
    size_t heap_size = 1 << *(uint8_t*) heapstart;

    for (block_t* block = (block_t*) heapstart + 2 + heap_size;
            (uint8_t*) block < prog_break;
            block++) {
        printf("%s %d\n",
                block->allocated ? "allocated" : "free",
                1 << block->size);
    }
}