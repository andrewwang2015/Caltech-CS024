#include <stdio.h>


int count_onebits(unsigned int n);


int main(int argc, char **argv) {
    int i, res;
    unsigned int n;

    if (argc == 1) {
        printf("usage:  %s N1 [N2 ...]\n\n", argv[0]);
        printf("\tN1, N2, ... are positive integers\n\n");
        printf("\tFor each argument, the program prints out the number of\n"
               "\tbits that are 1 in the number.\n\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
        res = sscanf(argv[i], "%u", &n);
        if (res == 1)
            printf("Input:  %u\tOne-bits:  %u\n\n", n, count_onebits(n));
        else
            printf("Unparseable input \"%s\".  Skipping.\n\n", argv[i]);
    }

    return 0;
}

/*
 * Given an unsigned integer n, this function returns the number of bits in n
 * that are 1 using the expression n = n & (n - 1).
 */
int count_onebits(unsigned int n) {
    unsigned int count = 0;
    /* We use a loop to track how many times we perform the operation n =
    n & (n-1). Because this expression will toggle all bits from the 
    rightmost occurence of a one bit (inclusive) to the 0th bit, by 
    keeping track of how many times we loop, we get precisely the number
    of one bits in n.*/

    while (n) {   
        count += 1;
        n &= (n - 1);
    }

    return count;
}