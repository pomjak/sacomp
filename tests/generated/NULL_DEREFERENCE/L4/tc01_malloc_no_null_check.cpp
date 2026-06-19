/* === MUTATOR: L4_loop_wrap ===
 * Source  : ./NULL_DEREFERENCE/tc01_malloc_no_null_check.cpp
 * Function: malloc_no_check_bad
 * ======================================================= */
/*
 * CWE-476: NULL Pointer Dereference
 * TC01 - malloc() can return NULL on allocation failure.
 *        Dereferencing the return value without a null check is UB.
 *
 * BAD:  return value of malloc used immediately without check.
 * GOOD: null check performed before dereference.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void malloc_no_check_bad(void)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    int *p = (int *)malloc(sizeof(int));
    /* BUG: p may be NULL; dereference without check */
    *p = 42;
    printf("*p = %d\n", *p);
    free(p);

    }
}
/* GOOD */
void malloc_no_check_good(void)
{
    int *p = (int *)malloc(sizeof(int));
    if (p == NULL) {
        fprintf(stderr, "allocation failed\n");
        return;
    }
    *p = 42;
    printf("*p = %d\n", *p);
    free(p);
}

int main(void)
{
    malloc_no_check_good();
    malloc_no_check_bad();
    return 0;
}
