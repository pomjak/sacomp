/* === MUTATOR: L5_combined ===
 * Source  : ./DOUBLE_FREE/tc03_alias_double_free.cpp
 * Function: alias_double_free_bad
 * ======================================================= */
/*
 * CWE-415: Double Free
 * TC03 - Two pointer variables point to the same allocation.
 *        Both are freed, causing a double free through aliasing.
 *
 * BAD:  alias = ptr, then both freed.
 * GOOD: only one of the two pointers is freed.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void alias_double_free_bad_impl()
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *ptr   = (char *)malloc(64);
    _alias_container.ptr = ptr;
    if (_alias_container.ptr == NULL) return;

    char *alias = _alias_container.ptr;   /* alias points to the same block */

    _alias_container.ptr[0] = 'A';
    printf("_alias_container.ptr[0] = %c\n", alias[0]);

    free(_alias_container.ptr);    /* first free */
    free(alias);  /* BUG: second free of the same block */
}

void alias_double_free_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    alias_double_free_bad_impl();

    }
}

/* GOOD */
void alias_double_free_good(void)
{
    char *ptr   = (char *)malloc(64);
    if (ptr == NULL) return;

    char *alias = ptr;

    ptr[0] = 'A';
    printf("ptr[0] = %c\n", alias[0]);

    free(ptr);
    /* alias is now dangling but we do not free it again */
    (void)alias;
}

int main(void)
{
    alias_double_free_good();
    alias_double_free_bad();
    return 0;
}
