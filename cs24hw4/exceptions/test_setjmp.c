#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "my_setjmp.h"
#include "test_setjmp.h"

/* This global_buf will be used to test jumping across multiple functions. */
static jmp_buf global_buf;

/*
 * This test makes sure that calling longjmp with an int argument set to 
 * zero causes setjmp() to return 1.
 *
 * Args: none
 *
 * Returns:
 *      1 if longjmp(buf, 0) returns 1 
 *      -1 otherwise 
 */
int longjmp_0_returns_1(void)
{
    jmp_buf buf;
    /* Set jump buffer to 0. */
    int res = setjmp(buf);
    if (res == 0) {
        /* Call longjmp with int. arg. of zero. */
        longjmp(buf, 0);
    } else if (res == 1) {
        return 1;
    } else {
        return -1;
    }

}


/*
 * This test makes sure that calling longjmp with an int argument set to 
 * a nonzero integer causes setjmp() to return this nonzero integer.
 *
 * Args:
 *      int n: the integer to pass arg. to longjmp
 *
 * Returns:
 *      1 if longjmp(buf, n) returns n for nonzero n
 *      -1 otherwise 
 */
int longjmp_n_returns_n(int n)
{
    jmp_buf buf;
    /* Set jump buffer to 0. */
    int res = setjmp(buf);
    if (res == 0) {
        /* Call longjmp with int. arg. of zero. */
        longjmp(buf, n);
    } else if (res == n) {
        return 1;
    } else {
        return -1;
    }
}

/*
 * This test makes sure that longjmp() can correctly jump across multiple
 * function invocations. We call setjmp() in this method and then invoke
 * a separate method that calls longjmp() and verifies that the caller has
 * its setjmp() return value changed correctly.
 *
 * Args:
 *      int n: the integer to pass arg. to longjmp
 *
 * Returns:
 *      1 if longjmp(buf, n) correctly sets the setjmp() return value across
 *           multiple functions.
 *      -1 otherwise 
 */
int jump_from(int n)
{
    int res = setjmp(global_buf);
    if (res == 0) {
        jump_to(n);
    } else if ((n == 0 && res == 1) || (n != 0 && res == n)) {
        /* If the invocated function correctly returns. */
        return 1;
    } else {
        return -1;
    }
}

/*
 * This function serves as the "called" function to test functionality of 
 * longjmp() across multiple function invocations.
 *
 * Args:
 *      int n: the integer to call longjmp with
 *
 * Returns:
 *      n/a
 *
 */
void jump_to(int n)
{
    longjmp(global_buf, n);
}

/* 
 * This function tests that the functions do not corrupt the stack in 
 * certain obvious ways. We put local variables with known values on both 
 * sides of our jmp_buf variable to ensure that our setjmp() implementation
 * does not go beyond the extent of the jmp_buf. 
 *
 * Args:
 *      n/a 
 *
 * Returns:
 *      1 if local variables remain unchanged after setjmp() and longjmp().
 *      -1 if any local variable has changed.
 *
 */


int test_stack_integrity(void)
{
    int before, before1, before2, after, after1, after2;
    /* Variables before jump buffer. */
    before = 1;
    before1 = 2;
    before2 = 3;
    jmp_buf buf;

    /* Variables after jump buffer. */
    after = 4;
    after1 = 5;
    after2 = 6;

    if(setjmp(buf) == 0) {
        longjmp(buf, 1);
    } else {
        if (before == 1 && before1 == 2 && before2 == 3 
            && after == 4 && after1 == 5 && after2 == 6) {
                return 1;
        }
    }
    return -1;
}


int main(void) 
{
    int total_tests = 5;

    srand(time(NULL));  /* Seed random integer generator. */

    /* longjmp(buf, 0) returns 1. */

    printf("longjmp(buf, 0) returns 1:      ");
    if (longjmp_0_returns_1() == 1) {
        printf("PASS\n");
        total_tests--;
    } else {
        printf("FAIL\n");
    }

    /* longjmp(buf, n) = n for random positive integer n. */
    int positive = rand() % (INT_MAX) + 1;
    printf("longjmp(env, %i) returns %i:        ", positive, positive);
    if (longjmp_n_returns_n(positive) == 1) {
        printf("PASS\n");
        total_tests--;
    } else {
        printf("FAIL\n");
    }

    /* longjmp(buf, n) = n for random negative integer n. */
    int negative = -(rand() % (INT_MAX) + 1);
    printf("longjmp(env, %i) returns %i:        ", negative, negative);
    if (longjmp_n_returns_n(negative) == 1) {
        printf("PASS\n");
        total_tests--;
    } else {
        printf("FAIL\n");
    }

    positive = rand() % (INT_MAX) + 1;
    negative = -(rand() % (INT_MAX) + 1);
    /* longjmp across multiple function invocations. */
    printf("longjmp() works across multiple functions with " 
        "arguments %i, %i, %i:        ", 0, positive, negative);
    if (jump_from(0) == 1 && jump_from(positive) == 1 
        && jump_from(positive) == 1) {
        printf("PASS\n");
        total_tests--;
    } else {
        printf("FAIL\n");
    }

    /* setjmp and longjmp maintain stack integrity. */
    printf("setjmp(), longjmp() maintain stack integrity:        ");
    if (test_stack_integrity()) {
        printf("PASS\n");
        total_tests--;
    } else {
        printf("FAIL\n");
    }

    printf("\n----------------------------------------\n");
    if (total_tests == 0) {
        printf("---------- ALL TESTS PASSED ------------\n");
    } else {
        printf("---------- SOME TESTS FAILED -----------\n");
    }
    printf("----------------------------------------\n");

    return 0;
}