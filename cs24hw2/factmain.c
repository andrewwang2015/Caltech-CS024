#include <stdio.h>
#include <stdlib.h>
#include "fact.h"

/* 
 * This program performs basic verification of the command line and then
 * calls the fact.s assembly function and finally prints the result. 
 * The verification process requires that the program only receive one
 * argument and that the numeric argument is nonnegative.
 */

int main(int argc, char **argv) {
    /* 
     * First check to make sure we only get one argument (two arguments
       including the name used to invoke the argument).
     */
    if (argc != 2) {
        fprintf(stderr, "Error: Program only takes 1 argument.\n");
        exit(EXIT_FAILURE);
    }
    else {
        /* 
         * If we have correct number of arguments, we need to ensure that 
        the argument is non-negative.
         */
        int arg = atoi(argv[1]);
        if (arg < 0) {
            fprintf(stderr, "Error: Argument must be nonnegative.\n");
            exit(EXIT_FAILURE);
        }
        
        printf("%d! = %d\n", arg, fact(arg));
        exit(EXIT_SUCCESS);
    }
}