/*
 * A simple test program for exercising the threads API. This program attempts
 * to pass an argument to the thread functions, to ensure that the argument 
 * is positioned properly in the thread's machine context such that the 
 * functions can retrieve and use it. 
 */

#include <stdio.h>
#include "sthread.h"


/*
 * This thread-function prints the value of argument and increments it before
 * yielding to another thread. 
 */

static void increment(void *arg) {
    while(1) {
        int passed_in = *((int *) arg);
        printf("Value of arg: %i\n", passed_in);
        passed_in++;
        *((int *) arg) = passed_in;
        sthread_yield();
    }
}

/*
 * This thread-function prints the value of argument and decrements it before
 * yielding to another thread. 
 */

static void decrement(void *arg) {
    while(1) {
        int passed_in = *((int *) arg);
        printf("Value of arg: %i\n", passed_in);
        passed_in--;
        *((int *) arg) = passed_in;
        sthread_yield();
    }
}

/*
 * Main function that creates two threads and makes sure they have access
 * to the arguments passed into them.
 */
int main(int argc, char **argv) {
    int arg = 0;
    sthread_create(increment, &arg);
    sthread_create(decrement, &arg);
    sthread_start();
    return 0;
}