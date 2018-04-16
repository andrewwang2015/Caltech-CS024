/* 
 * This file contains x86-64 assembly-language implementations of three
 * basic, very common math operations.
 *
 * As referenced in 3.6.6, the naive implementions for each of these 
 * functions would use conditional jump statements. These are not optimal
 * because for pipelining, it is desirable to determine the sequence of
 * instructions to be executed well ahead of time so that instructions can 
 * be executed efficiently. However, when a machine encounters a conditional
 * jump (or a branch), a halt is put into the process because the machine
 * cannot determine which way to branch until the condition is evaluated. To 
 * deal with this, processors employ branch prediction logic to keep the
 * instruction pipeline full of instructions, but if the processor mispredicts
 * a jump, then the future instructions that the processor put into the
 * pipeline are no longer valuable and thus must be discarded. Furthermore, 
 * the pipeline must now be filled with the instructions for the right branch.
 * All of this can decrease program performance, as evidenced by wasted clock
 * cycles. Thus, the best solution is to use conditional data transfers which
 * all of the below functions apply. In all the below cases, the processor 
 * can perform instructions (fill up the pipeline with instructions) without
 * worrying about having to undo "wrongly predicted" work as was the case with
 * conditional jump statements. Instead, with these conditional move 
 * instructions, the processor can execute these instructions without having 
 * to do any prediction of the result. All in all, each of these functions 
 * share the common theme of avoiding conditional control transfers to 
 * increase program efficiency.
 */

        .text

/*====================================================================
 * int f1(int x, int y)
 * This function returns min(x,y)
 */

.globl f1
f1:
        movl    %edi, %edx      # %edx = %edi = x
        movl    %esi, %eax      # %eax = %esi = y
        cmpl    %edx, %eax      # Compare %eax (y) and %edx (x), set flags
        cmovg   %edx, %eax      # If >=, %eax (return value) = x
        return                  # Return eax


/*====================================================================
 * int f2(int x)
 * This function returns the absolute value of x. These details are broken
 * down in the comments, but are summarized here. The important thing to
 * notice is the arithmetic shift right 31 times that basically gives a bit
 * mask of 0x00000000 if x is nonnegative and 0xffffffff if x is negative.
 * We XOR x with the bitmask. Now, if x was previously nonnegative, the result
 * of this would still be x (the bitmask is all zeroes). Now, if x was 
 * previously negative, then XORing with a bitmask of all ones would simply
 * reverse the bits of x (which if we convert to base 10, would equal
 * -x - 1). The last step is subtracting the bit mask from the result. Again,
 * if x was nonnegative, subtracting the bitmask will not change x. However
 * if x was previously negative, our bitmask is all 1s (decimal value of -1),
 * in which case, we would return (-x - 1 - (-1) = -x -1 + 1 = -x). Hence,
 * if x is nonnegative, we return x and if x is negative, we return -x.
 * This is the definition of abs(x). 
 */ 
.globl f2
f2:
        movl    %edi, %eax      # %eax = x

        movl    %eax, %edx      # %edx = %eax = x

        sarl    $31, %edx       # %edx >> = 31 ... 
                                # this yields a bit mask with all bits equal
                                # to the sign bit. 

        xorl    %edx, %eax      # %eax = %edx XOR %eax = %edx XOR x. Now, we
                                # XOR it with the bit mask. If x was positive
                                # the bitmask would be 0x00000000, and so
                                # %eax = x. However, if x was negative then
                                # our bitmask is 0xffffffff in which case,
                                # it would reverse all the bits of %eax so
                                # %eax = ~x

        subl    %edx, %eax      # %eax = %eax - %edx. This subtracts the bit
                                # mask from the result. If %eax was positive,
                                # then %eax remains unchanged. If %eax was 
                                # negative, then the reversed bits which,
                                # by definition is +x - 1
                                # gets 0xffff = -1 subtracted from it, which 
                                # means, we return x -1 - (-1) = x.
        ret                     # Return eax


/*====================================================================
 * int f3(int x)
 * This function returns 1 if x is negative and 0 if x is nonnegative. 
 * We can see this through the use of a bitmask. Initially, %eax holds 
 * a bit mask of 0x00000000 = 0 (base 10) (if x was nonnegative), and
 * 0xffffffffo (if x was negative). Next, we use testl to set the sign flag
 * based on the sign bit of %edx = x. Then, we set %edx = 1 and if the set 
 * sign flag is > 0, then we move %edx = 1 to %eax and return 1. Otherwise,
 * we do not move 1 into it, and return 0 (which was initially set). 
 */
.globl f3
f3:
        movl    %edi, %edx      # %edx = %edi = x
        movl    %edx, %eax      # %eax = %edx = x
        sarl    $31, %eax       # eax >>= 31 to generate bit mask consisting
                                # of bits equal to sign bit of x
        testl   %edx, %edx      # Sets sign flag to be that of sign of x
        movl    $1, %edx        # %edx = 1
        cmovg   %edx, %eax      # if sign flag = 1 > 0, then %eax = %edx = 1
        ret                     # return %eax

