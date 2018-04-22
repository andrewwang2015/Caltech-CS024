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

/* Size of the header (bytes) */
static unsigned const int HEADER = 4;

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
void init_myalloc() {

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
     * Initialize the initial state of your memory pool. This is an O(1)
     * operation given that we have the pointer to the beginning of our 
     * memory pool.
    */
    *((int *)mem) = MEMORY_SIZE - HEADER;

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
        shift_amount = HEADER + abs(remaining_space);

        /* This marks the location of next block's header. */
        iterator += shift_amount;
    }

    /* If we have a valid block to add to, we add. Else we return 0. */
    if (return_ptr) {
        /* 
         * Calculate amount of potential space if we were to split the block.
         * The amount of space left over is equal to the total remaining space
         * minus the size of first block to be inserted minus the amount of 
         * space for the next header. 
         */

        int space_for_second = min_block_size - size - HEADER;

        /* 
         * If we do not have space for second block, we mark the current
         * block as full. Otherwise, we split. 
         */
        iterator = return_ptr;
        if (space_for_second <= 0) {
            *((int *) iterator) = -remaining_space;
        } else {
            *((int *) iterator) = -size;
            iterator += (size + HEADER);
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
 * by myalloc(). Overall, this deallocation procedure takes linear time O(N)
 * where N is the number of allocated blocks. Coalescing the adjacent block
 * to the right is an O(1) operation because our header specifies how to get
 * to the next block, but coalescing the adjacent block to the left is an 
 * O(N) operation because we cannot get to the header of the previous block
 * without starting from the beginning of the memory pool and traversing
 * block by block.
 */
void myfree(unsigned char *oldptr) {
    /* 
     * oldptr signifies where the data starts, so we need to backtrack to get 
     * the header. 
     */
    iterator = oldptr;
    iterator -= HEADER;
    
    int free_space = abs(*((int *) iterator));

    /* 
     * Check the right adjacent block to see if coalescing is possible. Note 
     * that having the header allows us direct access to the adjacent right 
     * block, meaning coalescing to the right is an O(1) time operation.
     */
    iterator += (HEADER + free_space);

    /* Make sure we are not checking the adjacent block of the last block. */
    if (iterator < (mem + MEMORY_SIZE)) {
        int right_block_space = *((int *) iterator);
        /* 
         * Positive header means the block is free, and so we can coalesce. 
         * Do not forget to add in the header size as well.
         */
        if (right_block_space > 0) {
            free_space += (right_block_space + HEADER);
        }       
    }
    
    /* Change header of freed block to account for coalescing to the right. */
    *((int *) (oldptr - HEADER)) = free_space;

    /*
     * Now, we look to colaesce with the left adjacent block. We do this by
     * doing a linear traversal (O(N)) starting from the beginning and looking 
     * at adjacent blocks for possible coalescing. We know that by the way
     * we coalesce, we can break out of our traversal of blocks once we find 
     * a pair of adjacent blocks that both have positive space and can thus 
     * be coalesced. Otherwise, if we reach the end of the memory pool and 
     * find no pair of adjacent blocks with these properties, then that means
     * our original block to be freed cannot be coalesced with its adjacent
     * left block.
     */
    
     unsigned char *ptr_1 = mem;
     unsigned char *ptr_2 = mem + HEADER + abs(*((int *) ptr_1));

     /* Avoid accessing a header that is not within the block range. */
     while (ptr_2 < (mem + MEMORY_SIZE)) {
        int size_1 = *((int *) ptr_1);
        int size_2 = *((int *) ptr_2);
        /* If we have two adjacent blocks with free space, let's coalesce.*/
        if (size_1 > 0 && size_2 > 0) {
            int combined_size = size_1 + size_2;
            /* We also gain header space from combining two blocks to one. */
            *((int *) ptr_1) = combined_size + HEADER;
            break;
        } else {
            /* Otherwise, continue moving pointers forwards. */
            ptr_1 = ptr_2;
            ptr_2 = ptr_2 + HEADER + abs(*((int *) ptr_2));
        }
     }
      
    return;
}

/*!
 * Clean up the allocator state.
 * All this really has to do is free the user memory pool. This function mostly
 * ensures that the test program doesn't leak memory, so it's easy to check
 * if the allocator does.
 */
void close_myalloc() {
    free(mem);
}

/* 
 * Implements basic verification code to ensure heap is managed correctly. 
 * We do this by traversing all blocks in heap and computing the sum of 
 * allocated and free space; this sum should match the pool size. 
 */
void sanity_check() {
    /* Start at the beginning of memory pool. */
    iterator = mem;
    int total_space = 0;
    int num_blocks = 0;

    while (iterator < (mem + MEMORY_SIZE)) {
        /* Look at header and get the block size. */
        int block_size = abs(*((int *) iterator)) + HEADER;
        total_space += block_size;        
        num_blocks++;

        /* Print block number and header information for each block. */
        printf("Block %d: %d. ", num_blocks, *((int *) iterator));

        /* Move iterator to the next header. */
        iterator += block_size;
    }
    printf("Total space used: %d.\n", total_space);

    /* Make sure that the space usage adds up. */
    assert(total_space == MEMORY_SIZE);
}
