/* === MUTATOR: L5_combined ===
 * Source  : ./NULL_DEREFERENCE/tc04_null_after_reset.cpp
 * Function: reset_then_deref_bad
 * ======================================================= */
/*
 * CWE-476: NULL Pointer Dereference
 * TC04 - The pointer is explicitly set to NULL as a result of an error
 *        condition, and then used without rechecking.
 *        A common mistake when error-handling code reassigns a pointer.
 *
 * BAD:  ptr set to NULL in error branch, then used unconditionally below.
 * GOOD: ptr checked again before use.
 */
#include <cstdlib>
#include <cstdio>

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void reset_then_deref_bad_impl(int error)
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        int *ptr = nullptr;
    } _alias_container;

    int *ptr = (int *)malloc(sizeof(int));
    _alias_container.ptr = ptr;
    if (_alias_container.ptr == NULL) return;

    *_alias_container.ptr = 10;

    if (error) {
        free(_alias_container.ptr);
        _alias_container.ptr = NULL;   /* _alias_container.ptr is null now */
    }

    /* BUG: _alias_container.ptr might be NULL here if error was true */
    printf("value = %d\n", *_alias_container.ptr);
    if (_alias_container.ptr) free(_alias_container.ptr);
}

void reset_then_deref_bad(int error)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    reset_then_deref_bad_impl(error);

    }
}

/* GOOD */
void reset_then_deref_good(int error)
{
    int *ptr = (int *)malloc(sizeof(int));
    if (ptr == NULL) return;

    *ptr = 10;

    if (error) {
        free(ptr);
        ptr = NULL;
    }

    if (ptr != NULL) {           /* re-check before use */
        printf("value = %d\n", *ptr);
        free(ptr);
    } else {
        printf("ptr was reset due to error\n");
    }
}

int main(void)
{
    reset_then_deref_good(0);
    reset_then_deref_good(1);
    reset_then_deref_bad(1);
    return 0;
}
