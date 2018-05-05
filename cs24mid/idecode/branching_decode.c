/*! \file
 *
 * This file contains the definitions for the instruction decoder for the
 * branching processor.  The instruction decoder extracts bits from the current
 * instruction and turns them into separate fields that go to various functional
 * units in the processor.
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "branching_decode.h"
#include "register_file.h"
#include "instruction.h"


/*
 * Branching Instruction Decoder
 *
 *  The Instruction Decoder extracts bits from the instruction and turns
 *  them into separate fields that go to various functional units in the
 *  processor.
 */


/*!
 * This function dynamically allocates and initializes the state for a new 
 * branching instruction decoder instance.  If allocation fails, the program is
 * terminated.
 */
Decode * build_decode() {
    Decode *d = malloc(sizeof(Decode));
    if (!d) {
        fprintf(stderr, "Out of memory building an instruction decoder!\n");
        exit(11);
    }
    memset(d, 0, sizeof(Decode));
    return d;
}


/*!
 * This function frees the dynamically allocated instruction-decoder instance.
 */
void free_decode(Decode *d) {
    free(d);
}


/*!
 * This function decodes the instruction on the input pin, and writes all of the
 * various components to output pins.  Other components can then read their
 * respective parts of the instruction.
 *
 * NOTE:  the busdata_t type is defined in bus.h, and is a uint32_t
 */
void fetch_and_decode(InstructionStore *is, Decode *d, ProgramCounter *pc) {
    /* This is the current instruction byte we are decoding. */ 
    unsigned char instr_byte;

    /* The CPU operation the instruction represents.  This will be one of the
     * OP_XXXX values from instruction.h.
     */
    busdata_t operation;
    
    /* Source-register values, including defaults for src1-related values. */
    busdata_t src1_addr = 0, src1_const = 0, src1_isreg = 1;
    busdata_t src2_addr = 0;

    /* Destination register.  For both single-argument and two-argument
     * instructions, dst == src2.
     */
    busdata_t dst_addr = 0;
    
    /* Flag controlling whether the destination register is written to.
     * Default value is to *not* write to the destination register.
     */
    busdata_t dst_write = NOWRITE_REG;

    /* The branching address loaded from a branch instruction. */
    busdata_t branch_addr = 0;

    /* All instructions have at least one byte, so read the first byte. */
    ifetch(is);   /* Cause InstructionStore to push out the instruction byte */
    instr_byte = pin_read(d->input);

    /* 
     * First, get the opcode by logically shifting 4 bytes to the right. 
     * Because we know that instr_byte is unsigned, we are guaranteed that
     * after this shift, the top 4 bits will be zeroed out. 
     */
    operation = instr_byte >> OP_SHIFT;

    /* 
     * Now, switch on the op_code to determine what operation we need to do.
     * This will also give us insight on how many control instructions the 
     * op_code takes.
     */

    switch (operation) {
        /* If op_code is DONE, then we know we can break out. */
        case OP_DONE:
            break;

        /*
         * One argument instructions always occupy 1 byte. 
         * The encoding is [op3 op2 op1 op9 x src2 src1 src0].
         *      [op3 ... op0] are the bits of the opcode. 
         *      [src2 ... src0] are the bits of the register argument.
         */
        case OP_INC:
        case OP_DEC:
        case OP_INV:
        case OP_SHL:
        case OP_SHR:
            /* 
             * To get [src2 ... src0], let's do a bitwise and with 0x07. 
             * Also, we know that the destination and source are the same. 
             */
            src1_addr = src2_addr = dst_addr = instr_byte & REGISTER_MASK;

            /* Write to destination register. */
            dst_write = WRITE_REG;
            break;

        /* 
         * Two argument instructions always occupy 2 bytes. 
         * Byte 1 encoding: [op3 op2 op1 op9 a_isreg regb2 regb1 regb0]
         *      [regb2 ... regb0] are bits of second register argument
         * Byte 2 encoding: 
         *      when a_isreg == 1: [x x x x x rega2 rega1 rega0]
         *          [rega2 ... rega0] are bits of first register argument
         *      when a_isreg == 0: [8-bit constant]
         */
        case OP_MOV:
        case OP_ADD:
        case OP_SUB:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
            /* 
             * Decode the first byte. The destination is equal to the 
             * second source. 
             */
            dst_addr = src2_addr = instr_byte & REGISTER_MASK;
            src1_isreg = (instr_byte & ISREG_MASK);

            /* Fetch and decode the second byte. */
            incrPC(pc);
            ifetch(is);
            instr_byte = pin_read(d->input);

            if (src1_isreg) {
                /* If register, get bits of first register. */
                src1_addr = instr_byte & REGISTER_MASK;
            } else {
                src1_const = instr_byte;
            }
            /* Write to destination. */
            dst_write = WRITE_REG;
            break;

        /*
         * Branching instructions always occupy 1 byte. 
         * Encoding: [op3 op2 op1 op0 addr addr2 addr1 addr0]
         *      [op3 ... op0] are the bits of the opcode.
         *      [addr3 ... addr0] are bits of the address to branch to.
         */
        case OP_BRA:
        case OP_BRZ:
        case OP_BNZ:
            branch_addr = instr_byte & BRANCHING_MASK;
            break;

        default:
            fprintf(stderr, "Invalid opcode %d. \n", operation);
            break;
    }

    /* All decoded!  Write out the decoded values. */

    pin_set(d->cpuop,       operation);

    pin_set(d->src1_addr,   src1_addr);
    pin_set(d->src1_const,  src1_const);
    pin_set(d->src1_isreg,  src1_isreg);

    pin_set(d->src2_addr,   src2_addr);

    /* For this processor, like x86-64, dst is always src2. */
    pin_set(d->dst_addr,    src2_addr);
    pin_set(d->dst_write,   dst_write);

    pin_set(d->branch_addr, branch_addr);
}
