.globl __add_bigint

# This function takes bigint arrays a and b and computes b += a. A bigint
# variable n represents an arbitrary-size unsigned integer value, comprised
# of size 64-bit unsigned-int components such that n[0] is the low 64 bits
# of n, n[1] is the next 64 bits of n, and so on. This assumes that our 
# inpu/ arguments are valid because the function add_bigint function in 
# bigint.c handles useful error-checking.
#
# Arguments:
#   %rdi: pointer to array a 
#   %rsi: pointer to array b
#   %rdx/dl: size of array (unsigned byte)
#
# Returns: (int)
#   1 if addition was successful (no signed overflow), or if input size == 0
#   0 if addition was unsuccessful (signed overflow)

__add_bigint:
    cmp $0, %rdx
    je add_bigint_zero_size         # If size of array is 0, JUMP
    clc                             # Clear the carry flag
    movl $0, %ecx                   # Array index variable of input arrays

# This is responsible for the looping over all indices of a and b and doing
# the addition of corresponding elements. 
add_bigint_loop:
    movl $0, %r8d                   # Temporary register because we cannot 
                                    # directly add two memory references. 

    movq (%rdi, %rcx, 8), %r8       # Move a[%ecx] into %r8
    adc %r8, (%rsi, %rcx, 8)        # Add %r8 to b[%ecx]
    inc %ecx                        # Increase the array index variable. 

    dec %dl                         # This is the while loop condition. We  
                                    # keep decrementing size until we reach 0.

    jnz add_bigint_loop             # As long as we have remaining indices,
                                    # keep looping.

# This handles the returning of 1 if the completed addition was 
# successful and 0 if the completed addition resulted in an unsigned overflow.

add_bigint_done:
    jc   add_bigint_overflow         # Jump if overflow/carry
    movl $1, %eax                    # Otherwise, we move 1 for success
    ret

# This puts a 0 in the return register indicating that we had an overflow
# when we tried to perform addition.

add_bigint_overflow:
    movl $0, %eax                    # Return 0 in case of overflow
    ret

# This puts 1 into the return register and returns in the case
# where the input size is 0.

add_bigint_zero_size:  
    movl $1, %eax                     # Return 1 when size == 0
    ret
