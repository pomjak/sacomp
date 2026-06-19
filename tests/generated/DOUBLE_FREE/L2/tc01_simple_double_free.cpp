/* === MUTATOR: L2_aliasing ===
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
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *buf = (char *)malloc(128);
    _alias_container.ptr = buf;
    if (_alias_container.ptr == NULL) return;

    _alias_container.ptr[0] = 'X';
    free(_alias_container.ptr);
    /* BUG: _alias_container.ptr already freed */
    free(_alias_container.ptr);
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
