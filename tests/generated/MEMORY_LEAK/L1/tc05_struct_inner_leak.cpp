/* === MUTATOR: L1_interprocedural ===
 * Source  : ./MEMORY_LEAK/tc05_struct_inner_leak.cpp
 * Function: create_and_destroy_bad
 * ======================================================= */
/*
 * CWE-401: Memory Leak
 * TC05 - A struct contains a heap-allocated field.
 *        Freeing only the struct leaks the inner allocation.
 *        Tests whether analyzers track nested allocations.
 *
 * BAD:  free(node) without freeing node->data first.
 * GOOD: free(node->data) then free(node).
 */
#include <cstdlib>
#include <cstring>
#include <cstdio>

typedef struct {
    int  id;
    char *data;   /* heap-allocated field */
} Record;

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void create_and_destroy_bad_impl(int id, const char * payload)
{
    Record *rec = (Record *)malloc(sizeof(Record));
    if (rec == NULL) return;

    rec->id   = id;
    rec->data = (char *)malloc(strlen(payload) + 1);
    if (rec->data == NULL) { free(rec); return; }
    strcpy(rec->data, payload);

    printf("record %d: %s\n", rec->id, rec->data);

    /* BUG: rec->data is never freed */
    free(rec);
}


void create_and_destroy_bad(int id, const char *payload)
{
    create_and_destroy_bad_impl(id, payload);
}
/* GOOD */
void create_and_destroy_good(int id, const char *payload)
{
    Record *rec = (Record *)malloc(sizeof(Record));
    if (rec == NULL) return;

    rec->id   = id;
    rec->data = (char *)malloc(strlen(payload) + 1);
    if (rec->data == NULL) { free(rec); return; }
    strcpy(rec->data, payload);

    printf("record %d: %s\n", rec->id, rec->data);

    free(rec->data);   /* free inner field first */
    free(rec);
}

int main(void)
{
    create_and_destroy_bad(1, "hello");
    create_and_destroy_good(2, "hello");
    return 0;
}
