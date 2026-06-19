/* === MUTATOR: L5_combined ===
 * Source  : ./UNINITIALIZED_VALUE/tc01_simple_uninit_read.cpp
 * Function: compute_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC01 - A local integer is declared without initialization and
 *        immediately used in a computation.
 *
 * BAD:  x is used without assignment.
 * GOOD: x is initialized at declaration.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static int compute_bad_impl(int input)
{
    /* L2: alias transform skipped — no pointer decl found */

    int x;          /* uninitialized */
    int result = x + input;   /* BUG: x has indeterminate value */
    printf("result = %d\n", result);
    return result;
}

int compute_bad(int input)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    return compute_bad_impl(input);

    }
    return 0;
}

/* GOOD */
int compute_good(int input)
{
    int x = 0;      /* initialized */
    int result = x + input;
    printf("result = %d\n", result);
    return result;
}

int main(void)
{
    compute_good(5);
    compute_bad(5);
    return 0;
}
