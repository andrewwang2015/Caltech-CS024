/*! \file
 * Implementation of a simple memory allocator.  The allocator manages a small
 * pool of memory, provides memory chunks on request, and reintegrates freed
 * memory back into the pool.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2010.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "myalloc.h"


/*!
 * These variables are used to specify the size and address of the memory pool
 * that the simple allocator works against.  The memory pool is allocated within
 * init_myalloc(), and then myalloc() and free() work against this pool of
 * memory that mem points to.
 */
int MEMORY_SIZE;
unsigned char *mem;

/* 
 * For the header, a negative value indicates that the block is allocated
 * and a positive value indicates that the block is free. 
 */ 

/* Size of the header and footer (bytes) */
static unsigned const int HEADER = 4;
static unsigned const int FOOTER = 4;

/* Pointer that is used by many functions to traverse the blocks of memory. */
static unsigned char *iterator;


/*!
 * This function initializes both the allocator state, and the memory pool.  It
 * must be called before myalloc() or myfree() will work at all.
 *
 * Note that we allocate the entire memory pool using malloc().  This is so we
 * can create different memory-pool sizes for testing.  Obviously, in a real
 * allocator, this memory pool would either be a fixed memory region, or the
 * allocator would request a memory region from the operating system (see the
 * C standard function sbrk(), for example).
 */
void init_myalloc(void) {

    /*
     * Allocate the entire memory pool, from which our simple allocator will
     * serve allocation requests.
     */
    mem = (unsigned char *) malloc(MEMORY_SIZE);
    if (mem == 0) {
        fprintf(stderr,
                "init_myalloc: could not get %d bytes from the system\n",
		MEMORY_SIZE);
        abort();
    }

    /*
     * Initializes the header and footer of our memory pool. This is an O(1)
     * operation given that we have the pointer to the beginning of our 
     * memory pool.
    */
    *((int *) mem) = MEMORY_SIZE - HEADER - FOOTER;
    *((int *) (mem + MEMORY_SIZE - FOOTER)) = MEMORY_SIZE - HEADER - FOOTER;

}


/*!
 * Attempt to allocate a chunk of memory of "size" bytes.  Return 0 if
 * allocation fails. This uses a "best fit" strategy to allocate chunks
 * of memory meaning that a linear traversal of all the blocks is required
 * to find the smallest free block that can fit this new chunk of memory. 
 * Thus, the time complexity is O(N) where N is the number of blocks 
 * previously allocated in our memory pool.
 */
unsigned char *myalloc(int size) {

    iterator = mem;
    int remaining_space, shift_amount;
    unsigned char *return_ptr = NULL;

    /* 
     * This stores the size of the smallest free block seen which can fit 
     * our new chunk of memory. 
     */
    int min_block_size = MEMORY_SIZE;

    /* Linearly traverse all blocks to find "best fit". */
    while (iterator < (mem + MEMORY_SIZE)) {
        remaining_space = *((int *) iterator);

        /* Find the smallest free block that can fit the chunk of memory. */
        if (remaining_space >= size && remaining_space < min_block_size) {
            min_block_size = remaining_space;
            return_ptr = iterator;
        }
        /* This stores how many bytes to move over to get to next header. */
        shift_amount = HEADER + abs(remaining_space) + FOOTER ;

        /* This marks the location of next block's header. */
        iterator += shift_amount;
    }

    /* If we have a valid block to add to, we add. Else we return 0. */
    if (return_ptr) {
        /* 
         * Calculate amount of potential space if we were to split the block.
         * The amount of space left over is equal to the total remaining space
         * minus the size of first block to be inserted minus the amount of 
         * space for the next header and footer. 
         */

        int space_for_second = min_block_size - size - HEADER - FOOTER;

        /* 
         * If we do not have space for second block, we mark the current
         * block as full. Otherwise, we split. 
         */
        iterator = return_ptr;
        if (space_for_second <= 0) {

            /* Make remaining_space negative to denote it is now allocated. */
            *((int *) iterator) = -remaining_space;

        } else {

            /* Change header of current block. */
            *((int *) iterator) = -size;

            /* Change footer of current block. */
            iterator += (size + HEADER);
            *((int *) iterator) = -size;

            /* Initialize header of second block. */
            iterator += FOOTER;
            *((int *) iterator) = space_for_second;

            /* Initialize footer of second block. */
            iterator += (space_for_second + HEADER);
            *((int *) iterator) = space_for_second;
        }

        /* 
         * Set the return pointer to where the data starts for the block
         * inserted (move past the HEADER).
         */
        return_ptr += HEADER; 

    } else {
        fprintf(stderr, "myalloc: cannot service request of size %d with"
                " %lx bytes allocated\n", size, (iterator - mem));
        return (unsigned char *) 0;
    }

    return return_ptr;
}


/*!
 * Free a previously allocated pointer.  oldptr should be an address returned 
 * by myalloc(). This is a constant time deallocation because we have headers
 * and footers to help us look at the right block and left block, respectively
 * for coalescing. More specifically, for coalescing to the right, we simply
 * look at the header of the right/next block and check its sign to see if we
 * can merge it with the current block. For coalescing to the left, we look at 
 * the footer of the left/previous block and check its sign to see if we can
 * merge it with the current block. 
 */
void myfree(unsigned char *oldptr) {

    /* 
     * oldptr signifies where the data starts, so we need to backtrack to get 
     * the header. 
     */
    iterator = oldptr;
    iterator -= HEADER;

    /* 
     * If block was previously freed, meaning its header value is positive,
     * then throw an error.
     */ 

    int current_header = *((int *) iterator);
    if (current_header > 0) {
        fprintf(stderr, "myalloc: cannot free memory address "
            "previously freed.\n");
        exit(EXIT_FAILURE);
    }

    /*
     * This will store how much free space will exist after deallocation 
     * and coalescing. It will start off to just hold the value of the block
     * to be freed, but will be added to in cases of coalescing.
     */

    int free_space = abs(current_header);

    /* 
     * Check the right adjacent block to see if coalescing is possible. Note 
     * that having the header allows us direct access to the adjacent right 
     * block, meaning coalescing to the right is an O(1) time operation.
     */
    iterator += (HEADER + free_space + FOOTER);

    /* Make sure we are not checking the right block of the last block. */
    if (iterator < (mem + MEMORY_SIZE)) {
        int right_block_space = *((int *) iterator);
        /* 
         * Positive header means the block is free, and so we can coalesce. 
         * Do not forget to add in the header and footer size as well because
         * merging blocks means one less header and footer.
         */
        if (right_block_space > 0) {
            free_space += (right_block_space + HEADER + FOOTER);
        }       
    }
    
    /* 
     * Check the left adjacent block to see if coalescing is possible. Note 
     * that having the footer allows us direct access to the adjacent left 
     * block, meaning coalescing to the left is also an O(1) time operation.
     */

    /* Get to the footer of the block to the left. */
    iterator = oldptr - HEADER - FOOTER;
    
    /* 
     * This boolean will represent whether or not we coalece to the left. 
     * This will affect which block's header we modify.
     */

    int is_left_free = 0;

    /* 
     * This will store the size of the left block, so we can change the
     * left block's header if we end up coalescing to the left. 
     */
    int left_block_space;

    /* Make sure we are not checking the left block of the first block. */
    if (iterator > mem) {
        left_block_space = *((int *) iterator);
        /* 
         * Positive footer means the left block is free, and so we can 
         * coalesce. Again, merging blocks means we have additional space
         * from one less header and footer.
         */
        if (left_block_space > 0) {
            is_left_free = 1;
            free_space += (left_block_space + HEADER + FOOTER);
        } 
    }

    if (is_left_free) {
        /* 
         * Move iterator to start of header of left block. Before, iterator
         * was at the footer of the left block.
         */
        iterator -= (left_block_space + HEADER);
        *((int *) iterator) = free_space;

        /* Move iterator along to set the footer. */
        iterator += (HEADER + free_space);
        *((int *) iterator) = free_space;
    }
    else {
        /* 
         * In this case, we account for the scenarios of either coalescing to
         * the right or not colaescing to the right. Note that free_space
         * determines where we mark the footer so if we can coalesce to 
         * the right, then free_space would be larger to account for that.
         */

        /* Change header of current block to be freed. */
        *((int *) (oldptr - HEADER)) = free_space;

        /* Change footer. */
        *((int *) (oldptr + free_space)) = free_space;
    }

    return;
}

/*!
 * Clean up the allocator state.
 * All this really has to do is free the user memory pool. This function mostly
 * ensures that the test program doesn't leak memory, so it's easy to check
 * if the allocator does.
 */
void close_myalloc(void) {
    free(mem);
}

/* 
 * Implements basic verification code to ensure heap is managed correctly. 
 * We do this by traversing all blocks in heap and computing the sum of 
 * allocated and free space; this sum should match the pool size. In addition,
 * we output each block and its corresponding size. 
 */
void sanity_check(void) {
    /* Start at the beginning of memory pool. */
    iterator = mem;
    int total_space = 0;
    int num_blocks = 0;

    while (iterator < (mem + MEMORY_SIZE)) {
        /* Look at header and get the block size. */
        int block_size = abs(*((int *) iterator)) + HEADER + FOOTER;
        total_space += block_size;        
        num_blocks++;

        /* Print block number and header information for each block. */
        printf("Block %d: %d. ", num_blocks, *((int *) iterator));

        /* Move iterator to the next header. */
        iterator += (block_size);
    }

    printf("Total space used: %d.\n", total_space);

    /* Make sure that the space usage adds up. */
    assert(total_space == MEMORY_SIZE);
    return;
}
