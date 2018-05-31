/*
 * The simple thread scheduler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sthread.h"
#include "queue.h"
#include "timer.h"
#include "glue.h"


/*! The default stack size of threads, 1MiB of stack space. */
#define DEFAULT_STACKSIZE       (1 << 20)


/************************************************************************
 * Interface to the assembly.
 */


/*! This function must be called to start thread processing. */
void __sthread_start(void);


/*!
 * The is the entry point into the scheduler, to be called
 * whenever a thread action is required.
 */
void __sthread_switch(void);

/*!
 * Initialize the context for a new thread.
 *
 * The stackp pointer should point to the *end* of the area allocated for the
 * thread's stack.  (Don't forget that x86 stacks grow downward in memory.)
 *
 * The function f is initially called with the argument given.  When f
 * returns, __sthread_finish() is automatically called by the threading
 * library to ensure that the thread is cleaned up and deallocated properly.
 */
ThreadContext *__sthread_initialize_context(void *stackp, ThreadFunction f,
                                            void *arg);

/************************************************************************
 * Internal helper functions.
 */

/*
 * The initial thread context is initialized such that this function
 * will be called automatically when a thread's function returns.
 */
void __sthread_finish(void);

/*
 * This function deallocates the memory associated with a thread
 * when it terminates.
 */
void __sthread_delete(Thread *threadp);


/************************************************************************
 * Global variables and scheduler-level queue operations.
 */

/*!
 * The thread that is currently running.
 *
 * Invariant: during normal operation, there is exactly one thread in
 * the ThreadRunning state, and this variable points to that thread.
 *
 * Note that at start-up of the threading library, this will be NULL.
 */
static Thread *current;

/*!
 * The queue of ready threads.  Invariant:  All threads in the ready queue
 * are in the state ThreadReady.
 */
static Queue ready_queue;

/*!
 * The queue of blocked threads.  Invariant:  All threads in the blocked
 * queue are in the state ThreadBlocked.
 */
static Queue blocked_queue;


/*!
 * Add a thread to the appropriate scheduling queue, based on its state.
 */
static void enqueue_thread(Thread *threadp) {
    assert(threadp != NULL);

    switch(threadp->state) {
    case ThreadReady:
        queue_append(&ready_queue, threadp);
        break;
    case ThreadBlocked:
        queue_append(&blocked_queue, threadp);
        break;
    default:
        fprintf(stderr, "Thread state has been corrupted: %d\n", threadp->state);
        exit(1);
    }
}

/************************************************************************
 * Scheduler.
 */

/*
 * The scheduler is called with the context of the current thread,
 * or NULL when the scheduler is first started.
 *
 * The general operation of this function is:
 *   1.  Save the context argument into the current thread.
 *   2.  Either queue up or deallocate the current thread,
 *       based on its state.
 *   3.  Select a new "ready" thread to run, and set the "current"
 *       variable to that thread.
 *        - If no "ready" thread is available, examine the system
 *          state to handle this situation properly.
 *   4.  Return the context of the thread to run, so that a context-
 *       switch will be performed to start running the next thread.
 *
 * This function is global because it needs to be called from the assembly.
 */
ThreadContext *__sthread_scheduler(ThreadContext *context) {
    /* 
     * Upon entering the scheduler, we know that we are locked which is why
     * we do not have to explictly lock here. In all scenarios where 
     * we call __sthread_switch(), we lock beforehand and because 
     * __sthread_switch() calls __sthread_scheduler, we are guaranteed that 
     * upon entering this function, we are in a locked state. This is 
     * important because we want to ensure that the internal scheduler
     * state does not get completely mangled. In other words, all of the 
     * below operations are guaranteed to be atomic. 
     */ 

    if (context != NULL) {
        /* Add the current thread to the ready queue */
        assert(current != NULL);

        /* The thread could be Running, Blocked or Finished.
         * If Running, we must switch it back to Ready.
         */
        if (current->state == ThreadRunning)
            current->state = ThreadReady;

        /* If Running or Blocked, we just store the thread context and
         * re-queue the thread.  If Finished, then we need to deallocate
         * the thread's memory, because it is done.
         */
        if (current->state != ThreadFinished) {
            current->context = context;
            enqueue_thread(current);
        }
        else {
            __sthread_delete(current);
        }
    }

    /* Choose a new thread from the ready queue.  If there are no more
     * Ready threads, the system is either deadlocked, or we are finished
     * running and the program can be terminated.
     */
    current = queue_take(&ready_queue);
    if (current == NULL) {
        if (queue_empty(&blocked_queue)) {
            fprintf(stderr, "All threads completed, exiting.\n");
            exit(0);
        }
        else {
            fprintf(stderr, "The system is deadlocked!\n");
            exit(1);
        }
    }

    current->state = ThreadRunning;

    /* 
     * Upon exiting the scheduler, __sthread_restore is called which unlocks. 
     * Thus, there is no need to explicityl release the lock because now.
     */ 


    /* Return the next thread to resume executing. */
    return current->context;
}


/************************************************************************
 * Thread operations.
 */

/*
 * Start the scheduler.
 */
void sthread_start(int timer) {
    if (timer)
        start_timer();

    /* We are about to call a non-reentrant function __sthread_start() so
     * we must lock beforehand. This also ensures that upon entering 
     * __sthread_scheduler and __sthread_restore that it is locked so that
     * those atomic operations can be performed properly. 
     */
    __sthread_lock();

    __sthread_start();
    /* 
     * __sthread_start() ends up calling __sthread_restore which unlocks,
     * so no need to explicitly unlock here. 
     */
}

/*
 * Create a new thread.
 *
 * This function allocates a new context, and a new Thread
 * structure, and it adds the thread to the Ready queue.
 */
Thread * sthread_create(void (*f)(void *arg), void *arg) {
    /* 
     * No need to lock/unlock here because all threads are created before
     * sthread_start is called, so we do not have to worry about the timer 
     * interrupts in this function.
     */ 
    Thread *threadp;
    void *memory;

    /* Create a stack for use by the thread */
    memory = (void *) malloc(DEFAULT_STACKSIZE);
    if (memory == NULL) {
        fprintf(stderr, "Can't allocate a stack for the new thread\n");
        exit(1);
    }

    /* Create a thread struct */
    threadp = (Thread *) malloc(sizeof(Thread));
    if (threadp == NULL) {
        fprintf(stderr, "Can't allocate a thread context\n");
        free(memory);
        exit(1);
    }

    /* Initialize the thread */
    threadp->state = ThreadReady;
    threadp->memory = memory;
    threadp->context = __sthread_initialize_context(
        (char *) memory + DEFAULT_STACKSIZE, f, arg);
    enqueue_thread(threadp);

    return threadp;
}


/*
 * Return the pointer to the currently running thread.
 */
Thread * sthread_current(void) {
    return current;
}


/*
 * This function is called automatically when a thread's function returns,
 * so that the thread can be marked as "finished" and can then be reclaimed.
 * The scheduler will call __sthread_delete() on the thread before scheduling
 * a new thread for execution.
 *
 * This function is global because it needs to be referenced from assembly.
 */
void __sthread_finish(void) {
    /*
     * Lock the thread because we are making non-reentrant function calls to 
     * printf and __sthread_switch() (__sthread_switch calls 
     * __sthread_scheduler which is non-reentrant making __sthread_switch 
     * non-reentrant). We are also changing global state of current here, 
     * which we need to do atomically. 
     */
    __sthread_lock();
    printf("Thread %p has finished executing.\n", (void *) current);
    current->state = ThreadFinished;
    /* __sthread_switch() ends up unlocking the thread, so no need to here. */
    __sthread_switch();

}


/*!
 * This function is used by the scheduler to release the memory used by the
 * specified thread.  The function deletes the memory used for the thread's
 * context, as well as the memory for the Thread struct.
 */
void __sthread_delete(Thread *threadp) {
    assert(threadp != NULL);
    free(threadp->memory);
    free(threadp);
}


/*!
 * Yield, so that another thread can run.  This is easy: just
 * call the scheduler, and it will pick a new thread to run.
 */
void sthread_yield() {
    /* We need to lock here because __sthread_switch() is non-reentrant 
     * due to the fact that it calls __sthread_scheduler which is 
     * non-reentrant. 
     */
    __sthread_lock();
    /* sthread_switch() unlocks, so no need to do it here. */
    __sthread_switch();
}


/*!
 * Block the current thread.  Set the state of the current thread
 * to Blocked, and call the scheduler.
 */
void sthread_block() {
    /* 
     * We are changing global state of current here which we must do 
     * atomically. We also call a non-reentrant function __sthread_switch()
     * here because __sthread_switch() calls __sthread_scheduler which is
     * non-reentrant. Thus, we must lock.
     */ 
    __sthread_lock();
    current->state = ThreadBlocked;
    /* sthread_switch() unlocks, so no need to do it here. */
    __sthread_switch();
}


/*!
 * Unblock a thread that is blocked.  The thread is placed on
 * the ready queue.
 */
void sthread_unblock(Thread *threadp) {
    /* 
     * Lock here because we are changing the state of the blocked_queue and 
     * calling a non-renetrant function enqueue_thread. Thus, atomicity 
     * is required.
     */
    __sthread_lock();

    /* Make sure the thread was blocked */
    assert(threadp->state == ThreadBlocked);

    /* Remove from the blocked queue */
    queue_remove(&blocked_queue, threadp);

    /* Re-queue it */
    threadp->state = ThreadReady;
    enqueue_thread(threadp);

    /* 
     * We are done with the atomic steps. Also, unlike the previous functions
     * which end up calling __sthread_switch() which ends up unlocking, we
     * do not have that call here, so we must remember to unlock ourselves.
     */
    __sthread_unlock();
}

