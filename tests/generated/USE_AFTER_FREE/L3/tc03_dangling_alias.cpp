/* === MUTATOR: L3_opaque_branch ===
 * Source  : ./USE_AFTER_FREE/tc03_dangling_alias.cpp
 * Function: dangling_alias_bad
 * ======================================================= */
/*
 * CWE-416: Use After Free
 * TC03 - The owning pointer is freed via a second variable (alias),
 *        while the original pointer is subsequently used.
 *        Tests alias-aware UAF detection.
 *
 * BAD:  alias frees the block; original ptr is then dereferenced.
 * GOOD: use happens before any free.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
void dangling_alias_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    int *ptr   = (int *)malloc(sizeof(int) * 8);
    if (ptr == NULL) return;

    int *alias = ptr;   /* alias -> same block */

    for (int i = 0; i < 8; i++) ptr[i] = i;

    free(alias);   /* frees the shared block */

    /* BUG: ptr is now dangling */
    printf("ptr[0] = %d\n", ptr[0]);

    }
}
/* GOOD */
void dangling_alias_good(void)
{
    int *ptr   = (int *)malloc(sizeof(int) * 8);
    if (ptr == NULL) return;

    int *alias = ptr;

    for (int i = 0; i < 8; i++) ptr[i] = i;
    printf("ptr[0] = %d\n", ptr[0]);   /* use before any free */

    free(alias);
    (void)ptr;
}

int main(void)
{
    dangling_alias_good();
    dangling_alias_bad();
    return 0;
}
