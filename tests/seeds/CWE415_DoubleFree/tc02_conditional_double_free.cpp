/*
 * CWE-415: Double Free
 * TC02 - Pointer is freed inside a conditional branch AND again
 *        unconditionally after the branch.
 *
 * BAD:  free inside if-branch and again after the if block.
 * GOOD: free only in the else branch so exactly one free executes.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void conditional_double_free_bad(int flag)
{
    int *data = (int *)malloc(sizeof(int) * 16);
    if (data == NULL) return;

    if (flag) {
        printf("flag set, cleaning up early\n");
        free(data);   /* freed here ... */
    }

    /* BUG: also freed here regardless of flag */
    free(data);
}

/* GOOD */
void conditional_double_free_good(int flag)
{
    int *data = (int *)malloc(sizeof(int) * 16);
    if (data == NULL) return;

    if (flag) {
        printf("flag set, cleaning up early\n");
        free(data);
        data = NULL;
    }

    /* safe: free(NULL) is a no-op */
    free(data);
}

int main(void)
{
    conditional_double_free_good(1);
    conditional_double_free_good(0);
    /* conditional_double_free_bad(1); would crash */
    return 0;
}
