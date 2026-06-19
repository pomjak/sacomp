/* === MUTATOR: L3_opaque_branch ===
 * Source  : ./DOUBLE_FREE/tc01_simple_double_free.cpp
 * Function: double_free_bad
 * ======================================================= */
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
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    char *buf = (char *)malloc(128);
    if (buf == NULL) return;

    buf[0] = 'X';
    free(buf);
    /* BUG: buf already freed */
    free(buf);

    }
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
    double_free_good();
    double_free_bad(); 
    return 0;
}
