/*! \file
 *
 * This file contains definitions for an Arithmetic/Logic Unit of an
 * emulated processor.
 */


#include <stdio.h>
#include <stdlib.h>   /* malloc(), free() */
#include <string.h>   /* memset() */

#include "alu.h"
#include "instruction.h"


/*!
 * This function dynamically allocates and initializes the state for a new ALU
 * instance.  If allocation fails, the program is terminated.
 */
ALU * build_alu() {
    /* Try to allocate the ALU struct.  If this fails, report error then exit. */
    ALU *alu = malloc(sizeof(ALU));
    if (!alu) {
        fprintf(stderr, "Out of memory building an ALU!\n");
        exit(11);
    }

    /* Initialize all values in the ALU struct to 0. */
    memset(alu, 0, sizeof(ALU));
    return alu;
}


/*! This function frees the dynamically allocated ALU instance. */
void free_alu(ALU *alu) {
    free(alu);
}


/*!
 * This function implements the logic of the ALU.  It reads the inputs and
 * opcode, then sets the output accordingly.  Note that if the ALU does not
 * recognize the opcode, it should simply produce a zero result.
 */
void alu_eval(ALU *alu) {
    uint32_t A, B, aluop;
    uint32_t result;

    A = pin_read(alu->in1);
    B = pin_read(alu->in2);
    aluop = pin_read(alu->op);

    result = 0;
    switch (aluop) {
        case ALUOP_ADD:
            // Simple addition
            result = A + B;
            break;

        case ALUOP_INV:
            // Bitwise inversion, only to A
            result = ~A;
            break;

        case ALUOP_SUB:
            // Soimple subtraction
            result = A - B;
            break;

        case ALUOP_XOR:
            // Bitwise exclusive or
            result = A ^ B;
            break;

        case ALUOP_OR:
            // Bitwise or
            result = A | B;
            break;

        case ALUOP_INCR:
            // Incrementing a register
            result = A + 1;
            break;

        case ALUOP_AND:
            // Bitwise and
            result = A & B;
            break;

        case ALUOP_SRA:
            // Arithmetic shift right
            // We must ensure that the leading bit is preserved
            result = A >> 1;

            if ((A & 0x80000000) == 0)
            {
                // If leading bit of the source is 0, then we use a mask to 
                // make the leading bit of our result 0.
                result &= 0x7FFFFFFF;
            } else {
                // Leading bit of source is 1, so we ensure leading bit of
                // result is 1.
                result |= 0x80000000;
            }
            break;

        case ALUOP_SRL:
            // Logical shift right
            // Due to A already being unsigned, we know that just using the
            // >> operator will suffice in keeping the leading bit 0
            result = A >> 1;
            break;

        case ALUOP_SLA:
            // Arithmetic shift left 
            result = A << 1;
            break;

        case ALUOP_SLL:
            // Logical shift left
            result = A << 1;
            break;
        }

    pin_set(alu->out, result);
}

