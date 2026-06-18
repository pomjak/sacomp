/*
 * CWE-457: Use of Uninitialized Variable
 * TC01 - A local integer is declared without initialization and
 *        immediately used in a computation.
 *
 * BAD:  x is used without assignment.
 * GOOD: x is initialized at declaration.
 */
#include <cstdio>

/* BAD */
int compute_bad(int input)
{
    int x;          /* uninitialized */
    int result = x + input;   /* BUG: x has indeterminate value */
    printf("result = %d\n", result);
    return result;
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
    /* compute_bad(5); */
    return 0;
}
