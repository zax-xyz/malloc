#ifndef HELPERS_H
#define HELPERS_H

#include "virtual_alloc.h"

/**
 * Computes the base-2 logarithm of a given integer, giving the result as a
 * floor-rounded integer.
 */
uint8_t log_2(uint32_t x);

/**
 * Finds the smallest unallocated block in the virtual heap that is not smaller
 * than 2^min_size bytes. Modifies a pointer passed as a parameter to point to
 * the block in the heap and returns a pointer to the section of the heap
 * storing its information.
 */
block_t* smallest_block(void* heapstart, uint8_t min_size, uint8_t** ptr);

/**
 * Iteratively merges unallocated buddies to create bigger blocks until there
 * are no more buddies that can be merged with.
 */
int merge_blocks(void* heapstart, block_t* block, uint8_t* block_ptr);

/**
 * Returns whether a block is able to be merged to the left with its buddy, if
 * one exists. Assumes that the block in question is a right child.
 */
bool should_merge_left(block_t* block, uint8_t heap_size);

/**
 * Returns whether a block is able to be merged to the right with its buddy, if
 * one exists. Assumes that the block in question is a left child.
 */
bool should_merge_right(block_t* block, uint8_t heap_size);

/**
 * Tests whether a block is a left or right child in a binary tree
 * representation of the heap using binary search and returns the result.
 */
bool is_right(uint8_t size, uint8_t* heapstart, uint8_t* block_ptr);

/**
 * Moves everything in the heap from a starting position by an offset in bytes.
 */
void shift(block_t* block, uint8_t* prog_break, int16_t offset);

/**
 * Given a pointer to a block in the heap, finds the corresponding location
 * holding its information and returns it.
 */
block_t* get_block_info(void* heapstart, void* ptr);

#endif