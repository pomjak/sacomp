/* === MUTATOR: L4_loop_wrap ===
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
#include <cstdio>

static const int TABLE_SIZE = 8;
static const int table[8]   = {10, 20, 30, 40, 50, 60, 70, 80};

/* BAD */
void index_uninit_bad(void)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
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
