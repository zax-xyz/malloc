#include "virtual_alloc.h"

/*
 * Initialises the virtual heap with size 2^initial_size bytes, with minimum
 * block size 2^min_size. Resets the heap to an empty size before allocating
 * enough space for the heap and for information about the heap.
 */
void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
#ifdef DEBUG
    printf("INIT %d %d\n", initial_size, min_size);
#endif

    // we store the first block (full heap size) and 2 bytes for heap size and
    // minimum block size
    void* prog_break = virtual_sbrk(0);
    if (prog_break == (void*) -1)
        return;

    virtual_sbrk(heapstart - prog_break);  // reset heap
    // allocate space for heap, as well as 1 byte for first block information,
    // and storing initial_size and min_size
    virtual_sbrk((1 << initial_size) + 1 + 2);

    // store information about first block (free, full heap size)
    block_t* info_start = (block_t*) heapstart + 2 + (1 << initial_size);
    *info_start = (block_t) {false, initial_size};

    // store basic information about heap
    *(uint8_t*) heapstart = initial_size;
    *((uint8_t*) heapstart + 1) = min_size;
}

/*
 * Emulates malloc on the virtual heap. Follows the buddy allocation algorithm.
 * Allocates the block in the leftmost unallocated position that is sufficiently
 * large by splitting until reaching the desired size. Allocates blocks in sizes
 * of powers of 2. If allocation is not possible, returns NULL.
 */
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

    // block sizes have to be a power of 2 so take a log, however it also needs
    // to be at least min_size
    uint8_t needed_size = MAX(min_size, log_2(size));

    // keep track of pointer in heap for the block to allocate
    uint8_t* ptr = (uint8_t*) heapstart + 2;
    // find the leftmost block of the smallest size in the heap
    block_t* block = smallest_block(heapstart, needed_size, &ptr);
    if (block == NULL)
        // no valid unallocated block was found
        return NULL;

    // if the smallest valid block size is larger than what we need, we will
    // need to split blocks in half until we reach that size. this requires us
    // to expand the virtual heap to fit the information for the extra blocks
    uint8_t diff = block->size - needed_size;
    if (virtual_sbrk(diff) == (void*) -1)
        return NULL;

    uint8_t* prog_break = (uint8_t*) virtual_sbrk(0);
    if (prog_break == (uint8_t*) -1)
        return NULL;

    // if we need to split, move everything over to fit the extra blocks
    shift(block + 1, prog_break, diff);

    // split blocks and create extra unallocated blocks if needed
    for (uint8_t i = diff; i > 0; i--) {
        block->size--;
        *(block + i) = (block_t) {false, block->size};
    }

    block->allocated = true;

    return ptr;
}

/*
 * Emulates free on the virtual heap according to the buddy algorithm.
 * Unallocates a block pointed to by ptr and merges it with its buddy if the
 * buddy is also unallocated. Repeats the process until no longer possible.
 * Returns 0 if successful, 1 if not.
 */
int virtual_free(void* heapstart, void* ptr) {
#ifdef DEBUG
    printf("FREE %lu\n", (size_t)((uint8_t*) ptr - (uint8_t*) heapstart) - 2);
#endif

    // find the information about the block reference by ptr
    block_t* block = get_block_info(heapstart, ptr);
    if (block == NULL)
        // couldn't find the block, return error
        return 1;

    if (!block->allocated)
        // can't free this block, it's already unallocated
        return 1;

    // free the block and merge if needed according to the buddy algorithm
    block->allocated = false;
    return merge_blocks(heapstart, block, ptr);
}

/*
 * Emulates realloc on the virtual heap using the buddy allocation algorithm.
 * Attempts to resize a block to a specified size, moving it if necessary.
 * If the specified size is lower than the original size, truncates the data.
 * If the block is unable to be reallocated, the heap is left unchanged and NULL
 * is returned. Otherwise, a pointer to the new block is returned. If ptr is
 * NULL, 
 */
void* virtual_realloc(void* heapstart, void* ptr, uint32_t size) {
#ifdef DEBUG
    printf("REALLOC %lu %u\n", (uint8_t*) ptr - (uint8_t*) heapstart - 2, size);
#endif

    if (size == 0) {
        // if size is 0, behave as free
        virtual_free(heapstart, ptr);
        return NULL;
    }

    if (ptr == NULL)
        // if block pointer is NULL, behave as malloc
        return virtual_malloc(heapstart, size);

    uint8_t* prog_break = virtual_sbrk(0);
    if (prog_break == (uint8_t*) -1)
        return NULL;

    size_t heap_size = 1 << *(uint8_t*) heapstart;
    uint8_t* heap = (uint8_t*) heapstart + 2;

    if (size > heap_size)
        return NULL;
    
    // get information about this block
    block_t* block = get_block_info(heapstart, ptr);
    if (!block->allocated)
        // we can't realloc a block that isn't allocated
        return NULL;

    uint32_t og_size = 1 << block->size;

    // expand the virtual heap so that we can copy the blocks for backup
    if (virtual_sbrk(heap_size) == (void*) -1)
        return NULL;

    // backup the existing heap
    memmove(prog_break, heap, heap_size);

    // free the block to be reallocated
    if (virtual_free(heapstart, ptr))
        return NULL;

    // reallocate the block
    void* new_block = virtual_malloc(heapstart, size);

    // since free/malloc can change the size of the heap, we should recompute
    // where the backup of the heap is stored. it is always at the end, we just
    // need the start of it
    uint8_t* backup_heap = (uint8_t*) virtual_sbrk(0) - heap_size;

    // if reallocating failed, then copy the backup of the heap back to keep
    // everything the way it was before
    if (new_block == NULL) {
        memmove(heap, backup_heap, heap_size);
        virtual_sbrk(-heap_size);
        return NULL;
    }

    // otherwise if reallocation succeeded, copy the data into the new block
    memmove(new_block,
            backup_heap + ((uint8_t*) ptr - heap),
            MIN(og_size, size));

    // finally, reshrink the heap, getting rid of the backup
    if (virtual_sbrk(-heap_size) == (void*) -1)
        return NULL;

    return new_block;
}

/*
 * Prints information about each block in the heap, from left (smallest address)
 * to right. For each block, displays whether it is allocated or free, and its
 * size. The size is given as an exponent of 2. I.e., 2^size == "actual" size
 */
void virtual_info(void* heapstart) {
#ifdef DEBUG
    printf("INFO\n");
#endif

    uint8_t* prog_break = virtual_sbrk(0);
    if (prog_break == (uint8_t*) -1)
        return;

    size_t heap_size = 1 << *(uint8_t*) heapstart;

    for (block_t* block = (block_t*) heapstart + 2 + heap_size;
            (uint8_t*) block < prog_break;
            block++) {
        printf("%s %d\n",
                block->allocated ? "allocated" : "free",
                1 << block->size);
    }
}