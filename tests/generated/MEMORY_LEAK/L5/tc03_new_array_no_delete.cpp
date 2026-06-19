/* === MUTATOR: L5_combined ===
 * Source  : ./MEMORY_LEAK/tc03_new_array_no_delete.cpp
 * Function: build_matrix_bad
 * ======================================================= */
/*
 * CWE-401: Memory Leak
 * TC03 - C++ operator new[] used without a matching delete[].
 *        Tests whether analyzers handle C++ allocation operators.
 *
 * BAD:  array allocated with new[] is never deleted.
 * GOOD: delete[] called before function returns.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstring>
#include <cstdio>

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void build_matrix_bad_impl(int rows, int cols)
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        int *ptr = nullptr;
    } _alias_container;

    int *matrix = new int[rows * cols];
    _alias_container.ptr = matrix;
    memset(_alias_container.ptr, 0, sizeof(int) * rows * cols);
    printf("_alias_container.ptr[0][0] = %d\n", _alias_container.ptr[0]);
    /* missing: delete[] _alias_container.ptr; */
}

void build_matrix_bad(int rows, int cols)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    build_matrix_bad_impl(rows, cols);

    }
}

/* GOOD */
void build_matrix_good(int rows, int cols)
{
    int *matrix = new int[rows * cols];
    memset(matrix, 0, sizeof(int) * rows * cols);
    printf("matrix[0][0] = %d\n", matrix[0]);
    delete[] matrix;
}

int main(void)
{
    build_matrix_bad(4, 4);
    build_matrix_good(4, 4);
    return 0;
}
