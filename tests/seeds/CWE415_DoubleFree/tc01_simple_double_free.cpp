/*
 * CWE-415: Double Free
 * TC01 - The most direct form: free() called twice on the same pointer
 *        with no intervening reallocation.
 *
 * BAD:  free(buf) called twice.
 * GOOD: buf set to NULL after first free so second free is a no-op.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void double_free_bad(void)
{
    char *buf = (char *)malloc(128);
    if (buf == NULL) return;

    buf[0] = 'X';
    free(buf);
    /* BUG: buf already freed */
    free(buf);
}

/* GOOD */
void double_free_good(void)
{
    char *buf = (char *)malloc(128);
    if (buf == NULL) return;

    buf[0] = 'X';
    free(buf);
    buf = NULL;  /* nullify; free(NULL) is well-defined no-op */
    free(buf);
}

int main(void)
{
    double_free_good(/* call good first to avoid corruption */);
    /* double_free_bad(); */  /* commented out: would corrupt allocator in practice */
    return 0;
}
