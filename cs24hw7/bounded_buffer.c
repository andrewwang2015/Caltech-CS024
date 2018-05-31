/*
 * Define a bounded buffer containing records that describe the
 * results in a producer thread.
 *
 *--------------------------------------------------------------------
 * Adapted from code for CS24 by Jason Hickey.
 * Copyright (C) 2003-2010, Caltech.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "sthread.h"
#include "bounded_buffer.h"
#include "glue.h"
#include "semaphore.h"
#include "queue.h"

/*
 * The bounded buffer data.
 */
struct _bounded_buffer {
    /* The maximum number of elements in the buffer */
    int length;

    /* The index of the first element in the buffer */
    int first;

    /* The number of elements currently stored in the buffer */
    int count;

    /* The values in the buffer */
    BufferElem *buffer;

    /* 
     * Semaphore that keeps track of number of open slots. This is used
     * as a signal to producers so they know if there are slots in the 
     * buffer to add values to. 
     */
    Semaphore *open;

    /* 
     * Semaphore that keeps track of number of filled slots. This is used
     * as a signal to consumers so they know if there are values on the 
     * buffer that can be taken and printed.
     */
    Semaphore *filled;

    /* Semaphore that acts as a lock for atomic operations. This is going to
     * be binary where 1 means unlocked and 0 means locked.
     */
    Semaphore *lock;
};


#define EMPTY -1


/*
 * Allocate a new bounded buffer.
 */
BoundedBuffer *new_bounded_buffer(int length) {
    BoundedBuffer *bufp;
    BufferElem *buffer;
    int i;

    /* Allocate the buffer */
    buffer = (BufferElem *) malloc(length * sizeof(BufferElem));
    bufp = (BoundedBuffer *) malloc(sizeof(BoundedBuffer));
    if (buffer == 0 || bufp == 0) {
        fprintf(stderr, "new_bounded_buffer: out of memory\n");
        exit(1);
    }


    /* Initialize */

    memset(bufp, 0, sizeof(BoundedBuffer));

    for (i = 0; i != length; i++) {
        buffer[i].id = EMPTY;
        buffer[i].arg = EMPTY;
        buffer[i].val = EMPTY;
    }

    bufp->length = length;
    bufp->buffer = buffer;

    /* Allocate the three semaphores. On initialization, there are (length) 
     * number of open slots and 0 filled slots. The third semaphore, lock,
     * acts as a mutex. It is initialized to 1 which represents unlocked.
     */
    bufp->open = new_semaphore(length);
    bufp->filled = new_semaphore(0);
    bufp->lock = new_semaphore(1);

    /* new_semaphore already tests for when malloc fails. */

    return bufp;
}

/*
 * Add an integer to the buffer.  Yield control to another
 * thread if the buffer is full.
 */
void bounded_buffer_add(BoundedBuffer *bufp, const BufferElem *elem) {
    /*
     * We are adding to the bounded_buffer, so we need to wait for 
     * an open spot in the open semaphore. We also need to make sure
     * that the lock is "unlocked" so that we can guarantee atomicity.
     * More specifically, in case the timer interrupts in the middle
     * of this function and another thread gets switched to, we can be sure 
     * that the new thread will be locked out from doing anything which
     * allows the thread utilizing this function to assume running smoothly
     * when it gets switched back to. 
     */
    semaphore_wait(bufp->open);
    semaphore_wait(bufp->lock);

    /* Now the buffer has space.  Copy the element data over. */
    int idx = (bufp->first + bufp->count) % bufp->length;
    bufp->buffer[idx].id  = elem->id;
    bufp->buffer[idx].arg = elem->arg;
    bufp->buffer[idx].val = elem->val;

    bufp->count = bufp->count + 1;

    /* 
     * We are done with our atomic operations, so we can unlock now. This 
     * allows for interruption and other threads to run.
     */
    semaphore_signal(bufp->lock);
    /* 
     * We now have a filled slot so we signal the filled semaphore. This 
     * essentially notifies consumers that there is now an additional value
     * to take from the buffer.
     */
    semaphore_signal(bufp->filled);
}

/*
 * Get an integer from the buffer.  Yield control to another
 * thread if the buffer is empty.
 */
void bounded_buffer_take(BoundedBuffer *bufp, BufferElem *elem) {
    /*
     * Take requires that the filled semaphore be nonzero because it would 
     * be problematic to take from an empty buffer. Hence, we have to wait
     * until that happens. By similar logic to bounded_buffer_add,
     * we also need to make sure that the lock is "unlocked" so that we can 
     * guarantee atomicity.
     */
    semaphore_wait(bufp->filled);
    semaphore_wait(bufp->lock);

    /* Copy the element from the buffer, and clear the record */
    elem->id  = bufp->buffer[bufp->first].id;
    elem->arg = bufp->buffer[bufp->first].arg;
    elem->val = bufp->buffer[bufp->first].val;

    bufp->buffer[bufp->first].id  = -1;
    bufp->buffer[bufp->first].arg = -1;
    bufp->buffer[bufp->first].val = -1;

    bufp->count = bufp->count - 1;
    bufp->first = (bufp->first + 1) % bufp->length;

    /* 
     * We are done with our atomic operations, so we can unlock now. This 
     * allows for interruption and other threads to run.
     */
    semaphore_signal(bufp->lock);
    /* 
     * After taking from the bounded buffer, we have one more open slot. This
     * acts as a signal to producers that an additional spot opened up on
     * the buffer that can be filled.
     */
    semaphore_signal(bufp->open);
}

