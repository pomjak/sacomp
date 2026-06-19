/* === MUTATOR: L0_base ===
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
void alias_double_free_bad(void)
{
    char *ptr   = (char *)malloc(64);
    if (ptr == NULL) return;

    char *alias = ptr;   /* alias points to the same block */

    ptr[0] = 'A';
    printf("ptr[0] = %c\n", alias[0]);

    free(ptr);    /* first free */
    free(alias);  /* BUG: second free of the same block */
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
