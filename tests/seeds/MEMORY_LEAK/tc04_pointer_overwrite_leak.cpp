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
    char *ptr = (char *)malloc(32);
    if (ptr == NULL) return;

    /* BUG: original block leaked, ptr now points to new block */
    ptr = (char *)malloc(64);
    if (ptr == NULL) return;

    ptr[0] = 'A';
    printf("ptr[0] = %c\n", ptr[0]);
    free(ptr);
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
