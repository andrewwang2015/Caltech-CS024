#=============================================================================
# The gcd(a,b) function recursively computes the greatest commmon divisor
# between a and b using Euclid's algorithm.
#


.globl gcd 
gcd:
        movl    %edi, %eax      # Move a to %rax
        orl     $0, %esi        # Sets zero flag if b equals 0
        jnz     gcd_continue    # Computs gcd if nonzero
        jmp     gcd_return      # Else return a

gcd_continue:
        pushq   %rsi            # Save %rsi (b) for recursive call
        cqto                    # Converts quadword to octoword for division
        idivq   %rsi            # Divides a by b, stores remainder in %rdx
        movl    %edx, %esi      # a mod b (remainder) is 2nd arg. to rec. call
        popq    %rdi            # Pop b to %edi for 1st arg. to rec. call 
        call    gcd             # Make recursive call

gcd_return:
        ret     # All done
