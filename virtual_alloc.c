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

uint8_t log_2(uint32_t x) {
    uint8_t exp = 0;
    uint32_t counter = 1;
    while (counter < x) {
        exp++;
        counter <<= 1;
    }

    return exp;
}

uint8_t* smallest_block(void* heapstart, uint8_t min_size, uint8_t** ptr) {
    uint8_t lowest_size = UINT8_MAX;
    uint8_t* smallest_block = NULL;
    uint8_t* smallest_block_ptr = NULL;

    uint8_t heap_size = * (uint8_t*) heapstart;
    uint8_t* heap = (uint8_t*) heapstart + 2;

    uint8_t* info_start = heap + (1 << heap_size);

    uint8_t* block = info_start;

    for (; *ptr < info_start; *ptr += 1 << SIZE(*block), block++) {
        if (!ALLOCATED(*block) && SIZE(*block) >= min_size
                && SIZE(*block) < lowest_size) {
            lowest_size = SIZE(*block);
            smallest_block = block;
            smallest_block_ptr = *ptr;
        }
    }

    *ptr = smallest_block_ptr;

    return smallest_block;
}

void shift(uint8_t* block, uint8_t* prog_break, int16_t offset) {
    memmove(block + offset, block, prog_break - block);
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
    uint8_t* block = smallest_block(heapstart, needed_size, &ptr);
    if (block == NULL)
        return NULL;

    uint8_t diff = SIZE(*block) - needed_size;
    virtual_sbrk(diff);

    shift(block + 1, virtual_sbrk(0), diff);

    for (uint8_t i = diff; i > 0; i--) {
        DEC_SIZE(*block);
        *(block + i) = SIZE(*block) << 1;
    }

    SET_ALLOCATED(*block, true);

    return ptr;
}

bool should_merge_left(uint8_t* block) {
    uint8_t* left = block - 1;
    return !ALLOCATED(*left) && SIZE(*block) == SIZE(*left);
}

bool should_merge_right(uint8_t* block, uint8_t heap_size) {
    uint8_t* right = block + 1;
    return SIZE(*block) != heap_size && !ALLOCATED(*right)
           && SIZE(*block) == SIZE(*right);
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

uint8_t* merge_blocks(void* heapstart, uint8_t* block, uint8_t* block_ptr) {
    uint8_t* prog_break = virtual_sbrk(0);
    uint8_t heap_size = *(uint8_t*) heapstart;

    while (1) {
        bool right = is_right(SIZE(*block), (uint8_t*) heapstart, block_ptr);

        if (right && should_merge_left(block)) {
            INC_SIZE(*(block - 1));
            shift(block + 1, prog_break, -1);
            block--;
            block_ptr -= 1 << (SIZE(*block) - 1);
        } else if (!right && should_merge_right(block, heap_size)) {
            INC_SIZE(*(block + 1));
            shift(block + 1, prog_break, -1);
        } else {
            break;
        }

        // shrink heap
        virtual_sbrk(-1);
        prog_break -= 1;
    }

    return block;
}

uint8_t* get_block_info(void* heapstart, void* ptr) {
    uint8_t heap_size = *(uint8_t*) heapstart;

    uint8_t* blocks = (uint8_t*) heapstart + 2 + (1 << heap_size);
    uint8_t* block_ptr = heapstart + 2;

    for (uint8_t* block = blocks; block_ptr < blocks; block++) {
        if (block_ptr == ptr) {
            return block;
        }

        block_ptr += 1 << SIZE(*block);
    }

    return NULL;
}

int virtual_free(void* heapstart, void* ptr) {
#ifdef DEBUG
    printf("FREE %lu\n", (size_t)((uint8_t*) ptr - (uint8_t*) heapstart) - 2);
#endif

    uint8_t* block = get_block_info(heapstart, ptr);
    if (block == NULL)
        return 1;

    if (!ALLOCATED(*block))
        return 1;

    SET_ALLOCATED(*block, false);
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
    
    uint8_t* block = get_block_info(heapstart, ptr);
    uint32_t og_size = 1 << SIZE(*block);

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

    memmove(new_block, new_prog_break - heap_size - og_size, MIN(og_size, size));
    virtual_sbrk(-heap_size - og_size);

    return new_block;
}

void virtual_info(void* heapstart) {
#ifdef DEBUG
    printf("INFO\n");
#endif

    uint8_t* prog_break = virtual_sbrk(0);
    size_t heap_size = 1 << *(uint8_t*) heapstart;

    for (uint8_t* block = (uint8_t*) heapstart + 2 + heap_size;
            (uint8_t*) block < prog_break;
            block++) {
        printf("%s %d\n",
                ALLOCATED(*block) ? "allocated" : "free",
                1 << SIZE(*block));
    }
}
