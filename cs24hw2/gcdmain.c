#include <stdio.h>
#include <stdlib.h>
#include "gcd.h"

/* 
 * This program performs basic verification of the command line and then
 * calls the gcd.s assembly function and finally prints the GCD of the two
 * arguments passed in. The verification process requires that the program 
 * only receive two nonnegative arguments.
 */

int main(int argc, char **argv) {
    /* 
     * First check to make sure we only get two arguments(three arguments
       including the name used to invoke the argument).
     */
    if (argc != 3) {
        fprintf(stderr, "Error: Program only takes 2 arguments.\n");
        exit(EXIT_FAILURE);
    }
    else {
        /* 
         * If we have correct number of arguments, we need to ensure that 
         * the arguments are non-negative.
         */
        int arg1 = atoi(argv[1]);
        int arg2 = atoi(argv[2]);
        if (arg1 < 0 || arg2 < 0) {
            fprintf(stderr, "Error: Arguments must be nonnegative.\n");
            exit(EXIT_FAILURE);
        }
        
        /* 
         * Ensure the larger argument is first. 
         */
        if (arg1 < arg2){
            int temp = arg1;
            arg1 = arg2;
            arg2 = temp;
        }

        printf("GCD(%d, %d) = %d\n", arg1, arg2, gcd(arg1, arg2));
        exit(EXIT_SUCCESS);
    }
}