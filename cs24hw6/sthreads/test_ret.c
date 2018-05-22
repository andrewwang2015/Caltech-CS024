/*
 * A simple test program for exercising the threads API. This program creates
 * four threads, ensuring that each thread runs for a different length of time
 * and then terminates the threads by returning from the thread function. 
 */

#include <stdio.h>
#include "sthread.h"


/*
 * This thread-function prints the value of argument and increments it before
 * yielding to another thread. 
 */
static void loop(void * arg) {
    int iterations = *(int *) arg;
    for (int i = 0; i < iterations; i++) {
        printf("Iteration #: %i\n", i);
        sthread_yield();
    }
    printf("Loop of length %i finished.\n", iterations);
    return;
}

/*
 * Main function that creates four threads and starts execution of these 
 * threads that run for different lengths of time.
 */
int main(int argc, char **argv) {
    int length1 = 3;
    int length2 = 5;
    int length3 = 10;
    int length4 = 15;
    sthread_create(loop, &length1);
    sthread_create(loop, &length2);
    sthread_create(loop, &length3);
    sthread_create(loop, &length4);
    sthread_start();
    return 0;
}