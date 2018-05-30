/*
 * General implementation of semaphores.
 *
 *--------------------------------------------------------------------
 * Adapted from code for CS24 by Jason Hickey.
 * Copyright (C) 2003-2010, Caltech.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "sthread.h"
#include "semaphore.h"

/*
 * The semaphore data structure contains an integer i which represents
 * the semaphore invariant and a Queue that stores threads which are 
 * blocked so that the sempahore treats blocked threads fairly. 
 */
struct _semaphore {
    int i;
    Queue *blocked;
};

/************************************************************************
 * Top-level semaphore implementation.
 */

/*
 * Allocate a new semaphore.  The initial value of the semaphore is
 * specified by the argument.
 *
 * args:
 *      init: integer variable that represents the initial value of semaphore
 * returns:
 *      the newly created semaphore
 */
Semaphore *new_semaphore(int init) {

    /* Make sure init is nonnegative. */
    assert(init >= 0);

    /* Allocate memory for semaphore. */
    Semaphore *semp = malloc(sizeof(Semaphore));
    if (semp == NULL) {
        fprintf(stderr, "Can't allocate a stack for the new semaphore.\n");
        exit(1);
    }

    semp->i = init; 

    /* Allocate memory for queue. */
    Queue *q = malloc(sizeof(Queue));
    if (q == NULL) {
        fprintf(stderr, "Can't allocate a stack for blocked queue.\n");
        exit(1);
    }

    /* Initialize attributes of queue and set semaphore to contain queue. */
    q->head = NULL;
    q->tail = NULL;
    semp->blocked = q;
    
    return semp; 
}

/*
 * Decrement the semaphore.
 * This operation must be atomic, and it blocks iff the semaphore is zero.
 *
 * args:
 *      semp: the semaphore to decrement
 * returns: n/a
 */
void semaphore_wait(Semaphore *semp) {
    /* 
     * Lock thread because operation has to be atomic. These operations are 
     * concurrency primitives and multi-step operations so they must be 
     * executed atomically. 
     */
    __sthread_lock();

    /* While the semaphore value is zero, we have to continue blocking. */
    while (semp->i == 0) {
        /* Block the current thread and add to blocked queue of semaphore. */
        queue_append(semp->blocked, sthread_current());
        sthread_block();
    }

    /* 
     * sthread_block calls scheduler which ends up unlocking the thread upon
     * exiting the function. Thus we must lock the thread before changing 
     * the value of the semaphore.  
     */
    __sthread_lock();
    semp->i--;

}

/*
 * Increment the semaphore.
 * This operation must be atomic.
 *
 * args:
 *      semp: the semaphore to increment 
 * returns: n/a
 */
void semaphore_signal(Semaphore *semp) {
    /* 
     * Lock thread because operation has to be atomic. Again, by the same 
     * logic as semaphore_wait, we know that signalling which involves
     * testing and incrementing is a multi-step process and thus must be 
     * executed atomically. 
     */
    __sthread_lock();
    semp->i++;

    /* If a thread is blocked on semaphore, unlock one at head of queue. */
    if (!queue_empty(semp->blocked)) {
        sthread_unblock(queue_take(semp->blocked));
    }

    /* Unlock when done. All required atomic operations have completed. */
    __sthread_unlock();
}

