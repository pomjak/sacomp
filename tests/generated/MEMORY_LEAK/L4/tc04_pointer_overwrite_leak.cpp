/* === MUTATOR: L4_loop_wrap ===
 * Source  : ./MEMORY_LEAK/tc04_pointer_overwrite_leak.cpp
 * Function: overwrite_pointer_bad
 * ======================================================= */
/*
 * CWE-401: Memory Leak
 * TC04 - Pointer overwritten before the original allocation is freed.
 *        The first malloc'd block becomes unreachable when the pointer
 *        is reassigned.
 *
 * BAD:  ptr is reassigned; first allocation is lost.
 * GOOD: first allocation freed before reassignment.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void overwrite_pointer_bad(void)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    char *ptr = (char *)malloc(32);
    if (ptr == NULL) return;

    /* BUG: original block leaked, ptr now points to new block */
    ptr = (char *)malloc(64);
    if (ptr == NULL) return;

    ptr[0] = 'A';
    printf("ptr[0] = %c\n", ptr[0]);
    free(ptr);

    }
}
/* GOOD */
void overwrite_pointer_good(void)
{
    char *ptr = (char *)malloc(32);
    if (ptr == NULL) return;

    free(ptr);   /* free first block before reassigning */
    ptr = (char *)malloc(64);
    if (ptr == NULL) return;

    ptr[0] = 'A';
    printf("ptr[0] = %c\n", ptr[0]);
    free(ptr);
}

int main(void)
{
    overwrite_pointer_bad();
    overwrite_pointer_good();
    return 0;
}
