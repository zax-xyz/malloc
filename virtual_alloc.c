#include "virtual_alloc.h"

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
#ifdef DEBUG
    printf("INIT %d %d\n", initial_size, min_size);
#endif

    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    virtual_sbrk(heapstart - virtual_sbrk(0));  // reset heap
    virtual_sbrk((1 << initial_size) + sizeof(block_t) + 2);

    uint8_t* prog_break = (uint8_t*) heapstart + (1 << initial_size);

    *(block_t*) (prog_break + 2) = (block_t) {false, false, initial_size};

    *(uint8_t*) heapstart = initial_size;
    *((uint8_t*) heapstart + 1) = min_size;
}

uint8_t log_2(uint32_t x) {
    uint8_t exp = 0;
    uint32_t counter = 1;
    while (counter < x) {
        exp++;
        counter <<= 1;
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

void shift(block_t* block, uint8_t* prog_break, int16_t offset) {
    memmove(block + offset, block, prog_break - (uint8_t*) block);
}

uint8_t* get_block_of_size(void* heapstart, uint8_t size, block_t** block) {
    uint8_t* blocks_start = (uint8_t*) *block;
    for (uint8_t* block_ptr = (uint8_t*) heapstart + 2;
            block_ptr < blocks_start;
            block_ptr += 1 << (*block)->size, (*block)++) {
        if (!(*block)->allocated && (*block)->size == size) {
            return block_ptr;
        }
    }

    // This shouldn't happen as long as this function is used properly. That is,
    // using a correct size that was retrieved from smallest_block().
    return NULL;
}

void* virtual_malloc(void* heapstart, uint32_t size) {
#ifdef DEBUG
    printf("ALLOC %d\n", size);
#endif

    if (size == 0)
        return NULL;

    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(uint8_t*) heapstart;
    uint8_t min_size = *((uint8_t*) heapstart + 1);

    if (size > 1 << heap_size)
        return NULL;

    uint8_t needed_size = MAX(min_size, log_2(size));

    block_t* block = (block_t*) ((uint8_t*) heapstart + 2 + (1 << heap_size));

    uint8_t lowest_size = smallest_block(block, prog_break, needed_size);
    if (lowest_size == UINT8_MAX)
        return NULL;

    uint8_t diff = lowest_size - needed_size;
    virtual_sbrk(sizeof(block_t) * diff);

    uint8_t* block_ptr = get_block_of_size(heapstart, lowest_size, &block);
    if (block_ptr == NULL)
        return NULL;

    shift(block + 1, prog_break, diff);

    for (uint8_t i = diff; i > 0; i--) {
        block->size--;
        *(block + i) = (block_t) {false, true, block->size};
    }

    block->allocated = true;
    block->right = diff == 0;
    return block_ptr;
}

bool should_merge_left(block_t* block) {
    block_t* left = block - 1;
    return block->right && !left->allocated && block->size == left->size;
}

bool should_merge_right(block_t* block, uint8_t heap_size) {
    block_t* right = block + 1;
    return block->size != heap_size && !block->right && !right->allocated
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

block_t* merge_blocks(void* heapstart, block_t* block, uint8_t* block_ptr) {
    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(uint8_t*) heapstart;

    while (should_merge_left(block) || should_merge_right(block, heap_size)) {
        if (should_merge_left(block)) {
            (block - 1)->size++;
            shift(block + 1, prog_break, -1);
            block--;
            block_ptr -= 1 << (block->size - 1);
        } else if (should_merge_right(block, heap_size)) {
            (block + 1)->size++;
            shift(block + 1, prog_break, -1);
        }

        // shrink heap
        virtual_sbrk(-(int32_t) sizeof(block_t));
        prog_break -= sizeof(block_t);

        block->right = is_right(block->size, heapstart, block_ptr);
    }

    return block;
}

block_t* get_block_info(void* heapstart, void* ptr) {
    uint8_t heap_size = *(uint8_t*) heapstart;

    uint8_t* blocks = (uint8_t*) heapstart + 2 + (1 << heap_size);
    uint8_t* block_ptr = heapstart + 2;

    for (block_t* block = (block_t*) blocks; block_ptr < blocks; block++) {
        if (block_ptr == ptr) {
            return block;
        }

        block_ptr += 1 << block->size;
    }

    return NULL;
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
    uint8_t heap_size = 1 << *(uint8_t*) heapstart;
    
    block_t* block = get_block_info(heapstart, ptr);
    uint32_t og_size = 1 << block->size;

    virtual_sbrk(og_size);
    memmove(prog_break, ptr, og_size);
    virtual_info(heapstart);

    virtual_sbrk(heap_size);
    memmove(prog_break + og_size, heapstart, heap_size);

    virtual_info(heapstart);
    virtual_free(heapstart, ptr);
    virtual_info(heapstart);
    void* new_block = virtual_malloc(heapstart, size);
    virtual_info(heapstart);

    uint8_t* new_prog_break = virtual_sbrk(0);

    if (new_block == NULL) {
        memmove(heapstart, new_prog_break - heap_size, heap_size);
        virtual_sbrk(-heap_size - og_size);
        return NULL;
    }

    memmove(new_block, new_prog_break - heap_size - og_size, MIN(og_size, size));
    virtual_sbrk(-heap_size - og_size);

    return new_block;
}

void virtual_info(void* heapstart) {
#ifdef DEBUG
    printf("INFO\n");
#endif

    // uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = * (uint8_t*) heapstart;

    for (block_t* block = (block_t*) ((uint8_t*) heapstart + 2 + (1 << heap_size));
            (uint8_t*) block < (uint8_t*) heapstart + 2 + heap_size + 3;
            block++) {
#ifdef DEBUG
        printf("%s %d %s\n",
                block->allocated ? "allocated" : "free",
                1 << block->size,
                block->right ? "right" : "left");
#else
        printf("%s %d\n",
                block->allocated ? "allocated" : "free",
                1 << block->size);
#endif
    }
}
