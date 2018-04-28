.globl my_setjmp


# my_setjmp must store the callee-save registers: rbp, rbx, r12-r15, 
# current stack pointer, and the caller's return address. The input buffer
# is represented as an array, of size 8, of quadwords. This input buffer
# must be populated with the necessary information as to store the current
# information state.

# %rdi contains the buffer, or more specifically the address of the start of
# the buffer array.

my_setjmp:
    # First save the rbp, rpx, r12-r15 (callee-save registers)
    movq %rbp, (%rdi)            # Move base pointer to first arr slot
    movq %rbx, 8(%rdi)           # Move %rbx to second arr slot
    movq %r12, 16(%rdi)          # Move %r12 to third arr slot
    movq %r13, 24(%rdi)          # Move %r13 to fourth arr slot 
    movq %r14, 32(%rdi)          # Move %r14 to fifth arr slot 
    movq %r15, 40(%rdi)          # Move %r15 to sixth slot 

    # Move current stack pointer and caller's return address 
    movq %rsp, 48(%rdi)          # Move current stack pointer to seventh slot
    movq (%rsp), %r9             # Move caller's return address to eighth slot
    movq %r9, 56(%rdi)         

    # Set return value to 0.
    movl $0, %eax 
    ret


.globl my_longjmp

# my_longjmp restores the registers based on the buffers and reverts
# back to the how the stack was when my_setjmp was called. Returns 1 if
# the integer argument passed in is 0 else it returns the value of the
# argument.

# %rdi contains the buffer
# %rsi contains the integer argument 

my_longjmp:
    # Restore the rbp, rpx, r12-r15 (callee-save registers)
    movq (%rdi), %rbp             # Restore base pointer 
    movq 8(%rdi),%rbx             # Restore %rbx
    movq 16(%rdi), %r12           # Restore %r12
    movq 24(%rdi), %r13           # Restore %r13
    movq 32(%rdi), %r14           # Restore %r14
    movq 40(%rdi), %r15           # Restore %r15 

    # Restore previous stack pointer
    movq 48(%rdi), %rsp           # Restore stack pointer position
    movq 56(%rdi), %r9            # Restore caller's return address
    movq %r9, (%rsp)              # Make stack pointer point to return address

    # Set the return value 
    movl $1, %eax                 # Initially move 1 into %rax
    cmp  $0, %esi                 # Compare int. arg. to 0
    cmovne %esi, %eax             # If arg. not 0, then set %rax to be arg.
    ret
      
   
    