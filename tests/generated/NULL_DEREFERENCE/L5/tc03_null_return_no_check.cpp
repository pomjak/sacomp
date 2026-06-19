/* === MUTATOR: L5_combined ===
 * Source  : ./NULL_DEREFERENCE/tc03_null_return_no_check.cpp
 * Function: lookup_bad
 * ======================================================= */
/*
 * CWE-476: NULL Pointer Dereference
 * TC03 - A helper function explicitly documents that it can return NULL,
 *        but the caller dereferences the return value without checking.
 *        Tests whether the analyzer propagates nullability across a call.
 *
 * BAD:  find_item() return value dereferenced without null check.
 * GOOD: return value checked before dereference.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>

struct Item {
    int  id;
    char name[32];
};

static Item g_items[] = {
    {1, "alpha"},
    {2, "beta"},
    {3, "gamma"}
};

/* returns NULL if id not found */
static Item *find_item(int id)
{
    for (int i = 0; i < 3; i++) {
        if (g_items[i].id == id)
            return &g_items[i];
    }
    return NULL;   /* explicit NULL return documented */
}

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void lookup_bad_impl(int id)
{
    /* L2: alias transform skipped — no pointer decl found */

    Item *it = find_item(id);
    /* BUG: it may be NULL when id not in table */
    printf("found: %s\n", it->name);
}

void lookup_bad(int id)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    lookup_bad_impl(id);

    }
}

/* GOOD */
void lookup_good(int id)
{
    Item *it = find_item(id);
    if (it == NULL) {
        printf("id %d not found\n", id);
        return;
    }
    printf("found: %s\n", it->name);
}

int main(void)
{
    lookup_good(2);
    lookup_good(99);
    lookup_bad(99);
    return 0;
}
