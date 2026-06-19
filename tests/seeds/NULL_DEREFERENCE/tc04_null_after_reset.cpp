/*
 * CWE-476: NULL Pointer Dereference
 * TC04 - The pointer is explicitly set to NULL as a result of an error
 *        condition, and then used without rechecking.
 *        A common mistake when error-handling code reassigns a pointer.
 *
 * BAD:  ptr set to NULL in error branch, then used unconditionally below.
 * GOOD: ptr checked again before use.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void reset_then_deref_bad(int error)
{
    int *ptr = (int *)malloc(sizeof(int));
    if (ptr == NULL) return;

    *ptr = 10;

    if (error) {
        free(ptr);
        ptr = NULL;   /* ptr is null now */
    }

    /* BUG: ptr might be NULL here if error was true */
    printf("value = %d\n", *ptr);
    if (ptr) free(ptr);
}

/* GOOD */
void reset_then_deref_good(int error)
{
    int *ptr = (int *)malloc(sizeof(int));
    if (ptr == NULL) return;

    *ptr = 10;

    if (error) {
        free(ptr);
        ptr = NULL;
    }

    if (ptr != NULL) {           /* re-check before use */
        printf("value = %d\n", *ptr);
        free(ptr);
    } else {
        printf("ptr was reset due to error\n");
    }
}

int main(void)
{
    reset_then_deref_good(0);
    reset_then_deref_good(1);
    reset_then_deref_bad(1);
    return 0;
}
