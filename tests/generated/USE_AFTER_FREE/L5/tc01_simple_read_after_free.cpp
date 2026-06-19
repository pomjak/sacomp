/* === MUTATOR: L5_combined ===
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
/* L1: defect moved into helper — inter-procedural boundary */
static void read_after_free_bad_impl()
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *buf = (char *)malloc(32);
    _alias_container.ptr = buf;
    if (_alias_container.ptr == NULL) return;

    _alias_container.ptr[0] = 'A';
    free(_alias_container.ptr);

    /* BUG: _alias_container.ptr is dangling; reading it is undefined behaviour */
    printf("_alias_container.ptr[0] = %c\n", _alias_container.ptr[0]);
}

void read_after_free_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    read_after_free_bad_impl();

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
