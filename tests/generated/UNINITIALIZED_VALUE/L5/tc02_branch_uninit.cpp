/* === MUTATOR: L5_combined ===
 * Source  : ./UNINITIALIZED_VALUE/tc02_branch_uninit.cpp
 * Function: conditional_uninit_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC02 - Variable assigned inside an if-branch only.
 *        When the condition is false the variable is uninitialized
 *        but is still used afterwards.
 *
 * BAD:  status assigned only when flag != 0; used unconditionally.
 * GOOD: status has a default value before the conditional.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void conditional_uninit_bad_impl(int flag)
{
    /* L2: alias transform skipped — no pointer decl found */

    int status;   /* no default */

    if (flag) {
        status = 1;
    }
    /* BUG: status uninitialized when flag == 0 */
    printf("status = %d\n", status);
}

void conditional_uninit_bad(int flag)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    conditional_uninit_bad_impl(flag);

    }
}

/* GOOD */
void conditional_uninit_good(int flag)
{
    int status = 0;   /* safe default */

    if (flag) {
        status = 1;
    }
    printf("status = %d\n", status);
}

int main(void)
{
    conditional_uninit_good(0);
    conditional_uninit_good(1);
    conditional_uninit_bad(0);
    return 0;
}
