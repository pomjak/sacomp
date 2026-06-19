/* === MUTATOR: L3_opaque_branch ===
 * Source  : ./USE_AFTER_FREE/tc01_simple_read_after_free.cpp
 * Function: read_after_free_bad
 * ======================================================= */
/*
 * CWE-416: Use After Free
 * TC01 - Simplest case: a heap buffer is freed and then read.
 *
 * BAD:  buf is freed, then buf[0] is read.
 * GOOD: buf is read before it is freed.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void read_after_free_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    char *buf = (char *)malloc(32);
    if (buf == NULL) return;

    buf[0] = 'A';
    free(buf);

    /* BUG: buf is dangling; reading it is undefined behaviour */
    printf("buf[0] = %c\n", buf[0]);

    }
}
/* GOOD */
void read_after_free_good(void)
{
    char *buf = (char *)malloc(32);
    if (buf == NULL) return;

    buf[0] = 'A';
    printf("buf[0] = %c\n", buf[0]);  /* read before free */
    free(buf);
}

int main(void)
{
    read_after_free_good();
    read_after_free_bad();
    return 0;
}
