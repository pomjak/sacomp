/* === MUTATOR: L2_aliasing ===
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
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        int *ptr = nullptr;
    } _alias_container;

    int *ptr   = (int *)malloc(sizeof(int) * 8);
    _alias_container.ptr = ptr;
    if (_alias_container.ptr == NULL) return;

    int *alias = _alias_container.ptr;   /* alias -> same block */

    for (int i = 0; i < 8; i++) _alias_container.ptr[i] = i;

    free(alias);   /* frees the shared block */

    /* BUG: _alias_container.ptr is now dangling */
    printf("_alias_container.ptr[0] = %d\n", _alias_container.ptr[0]);
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
