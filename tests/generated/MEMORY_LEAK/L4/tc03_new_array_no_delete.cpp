/* === MUTATOR: L4_loop_wrap ===
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
#include <cstring>
#include <cstdio>

/* BAD */
void build_matrix_bad(int rows, int cols)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    int *matrix = new int[rows * cols];
    memset(matrix, 0, sizeof(int) * rows * cols);
    printf("matrix[0][0] = %d\n", matrix[0]);
    /* missing: delete[] matrix; */

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
