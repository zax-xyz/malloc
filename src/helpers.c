#include "virtual_alloc.h"

/**
 * Computes the base-2 logarithm of a given integer, giving the result as a
 * floor-rounded integer.
 */
uint8_t log_2(uint32_t x) {
    uint8_t exp = 0;
    uint32_t counter = 1;

    // calculate powers of 2 by bitshifting until we have reached the target
    while (counter < x) {
        exp++;
        counter <<= 1;
    }

    return exp;
}

/**
 * Finds the smallest unallocated block in the virtual heap that is not smaller
 * than 2^min_size bytes. Modifies a pointer passed as a parameter to point to
 * the block in the heap and returns a pointer to the section of the heap
 * storing its information.
 */
block_t* smallest_block(void* heapstart, uint8_t min_size, uint8_t** ptr) {
    uint8_t lowest_size = UINT8_MAX;
    block_t* smallest_block = NULL;
    uint8_t* smallest_block_ptr = NULL;

    uint8_t heap_size = * (uint8_t*) heapstart;

    block_t* info_start = (block_t*) heapstart + 2 + (1 << heap_size);
    block_t* block = info_start;

    // iterate through all the blocks in the heap, keeping track of our position
    // not only in the section with information of the blocks, but the blocks
    // themselves
    for (; *ptr < (uint8_t*) info_start; *ptr += 1 << block->size, block++) {
        // by ignoring any blocks of the same size as previously found, it is
        // guaranteed that we only record the leftmost blocks of any particular
        // size. thus we can optimise the algorithm to one pass instead of
        // repeatedly increasing the size to test
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

/**
 * Iteratively merges unallocated buddies to create bigger blocks until there
 * are no more buddies that can be merged with.
 */
int merge_blocks(void* heapstart, block_t* block, uint8_t* block_ptr) {
    uint8_t* prog_break = virtual_sbrk(0);
    if (prog_break == (uint8_t*) -1)
        return 1;

    uint8_t heap_size = *(uint8_t*) heapstart;
    uint8_t* heap = (uint8_t*) heapstart + 2;

    while (1) {
        // blocks only have buddies on one side, depending on if they're a
        // left child or a right child in a binary tree representation
        uint8_t* buddy = heap + ((block_ptr - heap) ^ (1 << block->size));

        if (buddy < block_ptr && should_merge_left(block, heap_size)) {
            block[-1].size++;
            shift(block + 1, prog_break, -1);

            // since we're merging left, the location of the block changes and
            // we have to update our pointers
            block--;
            block_ptr -= 1 << (block->size - 1);
        } else if (buddy > block_ptr && should_merge_right(block, heap_size)) {
            block[1].size++;
            shift(block + 1, prog_break, -1);
        } else {
            // we can no longer merge buddies, we are finished
            return 0;
        }

        // shrink heap
        if (virtual_sbrk(-1) == (void*) -1)
            return 1;

        // since we've merged 2 blocks together, the heap is now 1 block smaller
        prog_break -= 1;
    }
}

/**
 * Returns whether a block is able to be merged to the left with its buddy, if
 * one exists. Assumes that the block in question is a right child.
 */
bool should_merge_left(block_t* block, uint8_t heap_size) {
    block_t* left = block - 1;
    return !left->allocated && block->size == left->size;
}

/**
 * Returns whether a block is able to be merged to the right with its buddy, if
 * one exists. Assumes that the block in question is a left child.
 */
bool should_merge_right(block_t* block, uint8_t heap_size) {
    block_t* right = block + 1;
    return !right->allocated && block->size == right->size;
}

/**
 * Moves everything in the heap from a starting position by an offset in bytes.
 */
void shift(block_t* block, uint8_t* prog_break, int16_t offset) {
    memmove(block + offset, block, prog_break - (uint8_t*) block);
}

/**
 * Given a pointer to a block in the heap, finds the corresponding location
 * holding its information and returns it.
 */
block_t* get_block_info(void* heapstart, void* ptr) {
    uint8_t heap_size = *(uint8_t*) heapstart;

    block_t* start = (block_t*) heapstart + 2 + (1 << heap_size);
    uint8_t* block_ptr = heapstart + 2;

    for (block_t* block = start; block_ptr < (uint8_t*) start; block++) {
        if (block_ptr == ptr)
            return block;

        block_ptr += 1 << block->size;
    }

    // block was not found
    return NULL;
}