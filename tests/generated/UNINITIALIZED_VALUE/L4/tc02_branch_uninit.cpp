/* === MUTATOR: L4_loop_wrap ===
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
#include <cstdio>

/* BAD */
void conditional_uninit_bad(int flag)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    int status;   /* no default */

    if (flag) {
        status = 1;
    }
    /* BUG: status uninitialized when flag == 0 */
    printf("status = %d\n", status);

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
