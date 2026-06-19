/* === MUTATOR: L5_combined ===
 * Source  : ./USE_AFTER_FREE/tc05_loop_use_after_free.cpp
 * Function: loop_uaf_bad
 * ======================================================= */
/*
 * CWE-416: Use After Free
 * TC05 - A buffer is freed at the end of one loop iteration and then
 *        used (read) at the start of the next iteration.
 *        A simple loop-based UAF pattern.
 *
 * BAD:  ptr freed at end of iteration i, then read at start of iteration i+1.
 * GOOD: ptr reallocated at the start of each iteration before use.
 */
#include <cstdlib>
#include <cstdio>

#define ITERS 3

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void loop_uaf_bad_impl()
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *ptr = (char *)malloc(16);
    _alias_container.ptr = ptr;
    if (_alias_container.ptr == NULL) return;

    for (int i = 0; i < ITERS; i++) {
        /* BUG on i > 0: _alias_container.ptr was freed at end of previous iteration */
        _alias_container.ptr[0] = (char)('A' + i);
        printf("iter %d: %c\n", i, _alias_container.ptr[0]);
        free(_alias_container.ptr);   /* freed here; _alias_container.ptr is dangling for next iteration */
    }
}

void loop_uaf_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    loop_uaf_bad_impl();

    }
}

/* GOOD */
void loop_uaf_good(void)
{
    for (int i = 0; i < ITERS; i++) {
        char *ptr = (char *)malloc(16);  /* fresh allocation each iteration */
        if (ptr == NULL) return;

        ptr[0] = (char)('A' + i);
        printf("iter %d: %c\n", i, ptr[0]);
        free(ptr);
    }
}

int main(void)
{
    loop_uaf_good();
    loop_uaf_bad();
    return 0;
}
