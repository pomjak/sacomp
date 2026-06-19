/* === MUTATOR: L5_combined ===
 * Source  : ./NULL_DEREFERENCE/tc02_deref_before_null_check.cpp
 * Function: deref_before_check_bad
 * ======================================================= */
/*
 * CWE-476: NULL Pointer Dereference
 * TC02 - The programmer added a null check, but the pointer is
 *        dereferenced *before* the check is reached.
 *        Tests whether the analyzer tracks the ordering of operations.
 *
 * BAD:  *ptr used on the line before the if (ptr == NULL) guard.
 * GOOD: null check precedes every dereference.
 */
#include <cstdlib>
#include <cstdio>

static int *get_pointer(int flag)
{
    return flag ? (int *)malloc(sizeof(int)) : NULL;
}

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void deref_before_check_bad_impl(int flag)
{
    /* L2: alias transform skipped — no pointer decl found */

    int *ptr = get_pointer(flag);

    /* BUG: dereference before null check */
    printf("value = %d\n", *ptr);

    if (ptr == NULL) {   /* check is too late */
        return;
    }
    free(ptr);
}

void deref_before_check_bad(int flag)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    deref_before_check_bad_impl(flag);

    }
}

/* GOOD */
void deref_before_check_good(int flag)
{
    int *ptr = get_pointer(flag);

    if (ptr == NULL) {
        printf("pointer is null\n");
        return;
    }
    *ptr = 7;
    printf("value = %d\n", *ptr);
    free(ptr);
}

int main(void)
{
    deref_before_check_good(1);
    deref_before_check_good(0);
    deref_before_check_bad(0);
    return 0;
}
