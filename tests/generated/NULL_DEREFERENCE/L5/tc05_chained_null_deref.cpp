/* === MUTATOR: L5_combined ===
 * Source  : ./NULL_DEREFERENCE/tc05_chained_null_deref.cpp
 * Function: print_second_bad
 * ======================================================= */
/*
 * CWE-476: NULL Pointer Dereference
 * TC05 - A chain of struct pointer accesses where an intermediate
 *        pointer in the chain may be NULL.
 *        Tests whether the analyzer propagates nullability through
 *        field dereferences.
 *
 * BAD:  node->next->value accessed without checking node->next.
 * GOOD: node->next checked for NULL before accessing its fields.
 */
#include <cstdlib>
#include <cstdio>

struct ListNode {
    int value;
    ListNode *next;
};

static ListNode *make_node(int v, ListNode *n)
{
    ListNode *node = (ListNode *)malloc(sizeof(ListNode));
    if (node == NULL) return NULL;
    node->value = v;
    node->next  = n;
    return node;
}

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void print_second_bad_impl(ListNode * head)
{
    /* L2: alias transform skipped — no pointer decl found */

    /* BUG: head->next may be NULL (single-element list) */
    printf("second node value = %d\n", head->next->value);
}

void print_second_bad(ListNode *head)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    print_second_bad_impl(head);

    }
}

/* GOOD */
void print_second_good(ListNode *head)
{
    if (head == NULL || head->next == NULL) {
        printf("list has fewer than two elements\n");
        return;
    }
    printf("second node value = %d\n", head->next->value);
}

int main(void)
{
    ListNode *single = make_node(1, NULL);
    ListNode *pair   = make_node(1, make_node(2, NULL));

    if (single) {
        print_second_good(single);
        print_second_bad(single);
    }
    if (pair) {
        print_second_good(pair);
        free(pair->next);
        free(pair);
    }
    if (single) free(single);
    return 0;
}
