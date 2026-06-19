/* === MUTATOR: L4_loop_wrap ===
 * Source  : ./UNINITIALIZED_VALUE/tc01_simple_uninit_read.cpp
 * Function: compute_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC01 - A local integer is declared without initialization and
 *        immediately used in a computation.
 *
 * BAD:  x is used without assignment.
 * GOOD: x is initialized at declaration.
 */
#include <cstdio>

/* BAD */
int compute_bad(int input)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    int x;          /* uninitialized */
    int result = x + input;   /* BUG: x has indeterminate value */
    printf("result = %d\n", result);
    return result;

    }
}
/* GOOD */
int compute_good(int input)
{
    int x = 0;      /* initialized */
    int result = x + input;
    printf("result = %d\n", result);
    return result;
}

int main(void)
{
    compute_good(5);
    compute_bad(5);
    return 0;
}
