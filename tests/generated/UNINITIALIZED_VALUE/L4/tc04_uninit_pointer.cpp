/* === MUTATOR: L4_loop_wrap ===
 * Source  : ./UNINITIALIZED_VALUE/tc04_uninit_pointer.cpp
 * Function: uninit_pointer_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC04 - A pointer variable is declared but never assigned.
 *        It is then passed to a function that dereferences it.
 *        Tests uninitialized pointer detection (overlaps with CWE-476
 *        but the root cause here is lack of initialization, not a
 *        checked-null path).
 *
 * BAD:  ptr declared but not assigned; passed to sink().
 * GOOD: ptr initialized to a valid allocation before use.
 */
#include <cstdlib>
#include <cstdio>

static void sink(int *p)
{
    printf("value = %d\n", *p);   /* dereferences p */
}

/* BAD */
void uninit_pointer_bad(void)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    int *ptr;   /* uninitialized pointer */
    sink(ptr);  /* BUG: ptr has indeterminate value */

    }
}
/* GOOD */
void uninit_pointer_good(void)
{
    int value = 42;
    int *ptr = &value;   /* initialized */
    sink(ptr);
}

int main(void)
{
    uninit_pointer_good();
    uninit_pointer_bad();
    return 0;
}
