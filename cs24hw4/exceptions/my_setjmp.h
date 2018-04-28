#ifndef MY_SETJMP
#define MY_SETJMP

#include <stdint.h>

/*
 * #define to enable the use of setjmp()/longjmp() implementation in 
 * my_setjmp.s.
 *
 * #undef to stick with standard C implementation.
 */
#define ENABLE_MY_SETJMP


#ifdef ENABLE_MY_SETJMP

/*
 * We are daring and we're going to try out our own implementation of
 * setjmp() and longjmp()!  Watch out stack, here we come!
 */

/* Our jump buffer is of size 8. Each array slot is 8 bytes (64 bits). */
#define MY_JB_LEN 8
typedef uint64_t my_jmp_buf[MY_JB_LEN];

int my_setjmp(my_jmp_buf buf);
void my_longjmp(my_jmp_buf buf, int ret);

/*
 * Just in case these symbols were defined elsewhere, blow them away
 * and redefine them for ourselves.
 */

#undef jmp_buf
#undef setjmp
#undef longjmp

#define jmp_buf my_jmp_buf
#define setjmp my_setjmp
#define longjmp my_longjmp

#else

/*
 * We are going to stick with the standard implementation of setjmp()
 * and longjmp() for now...
 */
#include <setjmp.h>

#endif /* ENABLE_MY_SETJMP */

#endif /* MY_SETJMP */

