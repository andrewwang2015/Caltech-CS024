#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "ffunc.h"

/* This function takes an array of single-precision floating point values,
 * and computes a sum in the order of the inputs.  Very simple.
 */
float fsum(FloatArray *floats) {
    float sum = 0;
    int i;

    for (i = 0; i < floats->count; i++)
        sum += floats->values[i];

    return sum;
}


/* This function employs the "accurate summation with partials" approach
 * to compute summation of floats accurately. Pseudocode from 
 * code.activestate.com/recipies/393090/
 */
float my_fsum(FloatArray *floats) {
    float lo, hi, sum = 0.0;
    int i;

    // Because there is no "push_back/append" functionality in C, we 
    // need a variable to keep track of the "end" of the partials
    // array. 
    int partials_arr_size = 0;

    int num_elements = floats->count;
    float *values = floats->values;

    // Dynamically allocate a partials array. We know that, at most,
    // the size of the partials array will be equal to the number
    // of elements to add.
    float *partials = malloc(num_elements * sizeof(float));

    if (partials == NULL)
    {
        printf("Error! Memory not allocated.");
        exit(0);
    }

    for (int j = 0; j < num_elements; j++)
    {
        // Start at the beginning of the partials array for each new
        // element to be added.
        i = 0;
        float x = values[j];

        // For each element of the input arr, we add it to each partial  
        // in the array of partials. 
        // Applying hi/lo summation to each partial allows the array of 
        // partial sums to remain exact. 

        for (int k = 0; k < partials_arr_size; k++)
        {
            float y = partials[k];

            // Ensure that x >= y
            if (fabs(x) < fabs(y))
            {
                float temp = x;
                x = y;
                y = temp;
            }

            // The key to the algorithm is here. 
            // The rounded (x+y) is stored in hi while the round-off is
            // stored in lo. Hence, we have that (x+y) = (hi + lo).
            hi = x + y;
            lo = y - (hi - x);

            // If there is round-off, we must store it
            if (lo)
            {
                partials[i] = lo;
                i++;
            }

            // We update x, because as we traverse through the partials
            // array, we know that as long as we store the list of round-offs,
            // we can keep a running sum of the rounded sums.
            x = hi;
        }
        // After iterating through all the partials and storing the rounded
        // off portions, we got to store the rounded portion. 
        partials[i] = x;

        // Update the "end point" of the partials arr.
        partials_arr_size = i + 1;
    }

    // Sum over the partials array to get total sum
    for (int i = 0; i < partials_arr_size; i++)
    {
        sum += partials[i];
    }

    // Free dynamically allocated partials array
    free(partials);

    return sum;
}


int main() {
    FloatArray floats;
    float sum1, sum2, sum3, my_sum;

    load_floats(stdin, &floats);
    printf("Loaded %d floats from stdin.\n", floats.count);

    /* Compute a sum, in the order of input. */
    sum1 = fsum(&floats);

    /* Use my_fsum() to compute a sum of the values.  Ideally, your
     * summation function won't be affected by the order of the input floats.
     */
    my_sum = my_fsum(&floats);

    /* Compute a sum, in order of increasing magnitude. */
    sort_incmag(&floats);
    sum2 = fsum(&floats);

    /* Compute a sum, in order of decreasing magnitude. */
    sort_decmag(&floats);
    sum3 = fsum(&floats);

    /* %e prints the floating-point value in full precision,
     * using scientific notation.
     */
    printf("Sum computed in order of input:  %e\n", sum1);
    printf("Sum computed in order of increasing magnitude:  %e\n", sum2);
    printf("Sum computed in order of decreasing magnitude:  %e\n", sum3);
    printf("My sum:  %e\n", my_sum);

    /* Free the FloatArray */
    free(floats.values);

    return 0;
}

