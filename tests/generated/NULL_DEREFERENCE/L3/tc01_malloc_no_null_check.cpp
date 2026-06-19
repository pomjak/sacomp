/* === MUTATOR: L3_opaque_branch ===
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
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
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
