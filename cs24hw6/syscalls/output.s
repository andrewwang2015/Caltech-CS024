.globl output

# This outputs a data buffer to stdout (the standard output of the program).
# Traps into the kernel and invokes the write() system call to stdout.
# The write() call has form:
# ssize_t write (int fd, const void *buf, size_t count)
#   - fd is the "file descriptor" of the file to write to. Pass 1 for this.
#   - buf is the address of the data buffer to write out
#   - count is the number of bytes to write.
# In addition, to invoke a system call, %rax contains the number of system
# call to invoke, %rdi contains argument 1, %rsi contain argument 2, and
# %rdx contains argument 3.
#
# Arguments:
#   %rdi: pointer to msg (uint64_t array) / address of buffer to write out
#   %rsi: sizeof(msg) / number of bytes to write
#
# Returns: void

output:
    movq $1, %rax           # Move 1 (system-call number of write) into %rax
    pushq %rdi              # Push %rdi (address of buffer) onto stack
    movq $1, %rdi           # Move 1 into rdi for fd argument to write
    pushq %rsi              # Push %rsi (number of bytes to write) onto stack
    popq %rdx               # Pop number of bytes to write to %rdx (arg. 3)
    popq %rsi               # Pop address of buffer into %rsi (arg. 2)
    syscall 
    ret

    