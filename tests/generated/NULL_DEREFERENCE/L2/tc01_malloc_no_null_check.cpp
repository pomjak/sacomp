/* === MUTATOR: L2_aliasing ===
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
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        int *ptr = nullptr;
    } _alias_container;

    int *p = (int *)malloc(sizeof(int));
    _alias_container.ptr = p;
    /* BUG: _alias_container.ptr may be NULL; dereference without check */
    *_alias_container.ptr = 42;
    printf("*_alias_container.ptr = %d\n", *_alias_container.ptr);
    free(_alias_container.ptr);
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
