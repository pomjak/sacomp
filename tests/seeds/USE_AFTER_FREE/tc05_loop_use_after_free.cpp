/*
 * CWE-416: Use After Free
 * TC05 - A buffer is freed at the end of one loop iteration and then
 *        used (read) at the start of the next iteration.
 *        A simple loop-based UAF pattern.
 *
 * BAD:  ptr freed at end of iteration i, then read at start of iteration i+1.
 * GOOD: ptr reallocated at the start of each iteration before use.
 */
#include <cstdlib>
#include <cstdio>

#define ITERS 3

/* BAD */
void loop_uaf_bad(void)
{
    char *ptr = (char *)malloc(16);
    if (ptr == NULL) return;

    for (int i = 0; i < ITERS; i++) {
        /* BUG on i > 0: ptr was freed at end of previous iteration */
        ptr[0] = (char)('A' + i);
        printf("iter %d: %c\n", i, ptr[0]);
        free(ptr);   /* freed here; ptr is dangling for next iteration */
    }
}

/* GOOD */
void loop_uaf_good(void)
{
    for (int i = 0; i < ITERS; i++) {
        char *ptr = (char *)malloc(16);  /* fresh allocation each iteration */
        if (ptr == NULL) return;

        ptr[0] = (char)('A' + i);
        printf("iter %d: %c\n", i, ptr[0]);
        free(ptr);
    }
}

int main(void)
{
    loop_uaf_good();
    /* loop_uaf_bad(); */
    return 0;
}
