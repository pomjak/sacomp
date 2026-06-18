/*
 * CWE-476: NULL Pointer Dereference
 * TC02 - The programmer added a null check, but the pointer is
 *        dereferenced *before* the check is reached.
 *        Tests whether the analyzer tracks the ordering of operations.
 *
 * BAD:  *ptr used on the line before the if (ptr == NULL) guard.
 * GOOD: null check precedes every dereference.
 */
#include <cstdlib>
#include <cstdio>

static int *get_pointer(int flag)
{
    return flag ? (int *)malloc(sizeof(int)) : NULL;
}

/* BAD */
void deref_before_check_bad(int flag)
{
    int *ptr = get_pointer(flag);

    /* BUG: dereference before null check */
    printf("value = %d\n", *ptr);

    if (ptr == NULL) {   /* check is too late */
        return;
    }
    free(ptr);
}

/* GOOD */
void deref_before_check_good(int flag)
{
    int *ptr = get_pointer(flag);

    if (ptr == NULL) {
        printf("pointer is null\n");
        return;
    }
    *ptr = 7;
    printf("value = %d\n", *ptr);
    free(ptr);
}

int main(void)
{
    deref_before_check_good(1);
    deref_before_check_good(0);
    /* deref_before_check_bad(0); */
    return 0;
}
