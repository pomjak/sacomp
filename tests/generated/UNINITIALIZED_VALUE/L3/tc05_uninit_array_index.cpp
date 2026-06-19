/* === MUTATOR: L3_opaque_branch ===
 * Source  : ./UNINITIALIZED_VALUE/tc05_uninit_array_index.cpp
 * Function: index_uninit_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC05 - An uninitialized integer is used as an array index.
 *        This combines CWE-457 with a potential out-of-bounds access;
 *        the root defect tracked here is the uninitialized index.
 *
 * BAD:  idx is uninitialized; used to index into table[].
 * GOOD: idx is initialized before use.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>

static const int TABLE_SIZE = 8;
static const int table[8]   = {10, 20, 30, 40, 50, 60, 70, 80};

/* BAD */
void index_uninit_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    int idx;   /* uninitialized */
    /* BUG: idx may be any value; array access is UB */
    printf("table[idx] = %d\n", table[idx % TABLE_SIZE]);

    }
}
/* GOOD */
void index_uninit_good(int idx)
{
    if (idx < 0 || idx >= TABLE_SIZE)
        idx = 0;
    printf("table[%d] = %d\n", idx, table[idx]);
}

int main(void)
{
    index_uninit_good(3);
    index_uninit_bad();
    return 0;
}
