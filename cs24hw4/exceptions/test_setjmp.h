/* Tests that longjmp with zero as argument returns 1. */
int longjmp_0_returns_1(void);

/* Tests that longjmp with nonzero argument n returns n. */
int longjmp_n_returns_n(int n);

/* These two functions test longjmp across different functions. */
int jump_from(int n);
void jump_to(int n);

/* Tests that setjmp and longjmp maintain stack integrity. */
int test_stack_integrity(void);
