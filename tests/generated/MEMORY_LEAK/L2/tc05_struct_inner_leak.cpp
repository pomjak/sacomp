/* === MUTATOR: L2_aliasing ===
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
void create_and_destroy_bad(int id, const char *payload)
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        Record *ptr = nullptr;
    } _alias_container;

    Record *rec = (Record *)malloc(sizeof(Record));
    _alias_container.ptr = rec;
    if (_alias_container.ptr == NULL) return;

    _alias_container.ptr->id   = id;
    _alias_container.ptr->data = (char *)malloc(strlen(payload) + 1);
    if (_alias_container.ptr->data == NULL) { free(_alias_container.ptr); return; }
    strcpy(_alias_container.ptr->data, payload);

    printf("record %d: %s\n", _alias_container.ptr->id, _alias_container.ptr->data);

    /* BUG: _alias_container.ptr->data is never freed */
    free(_alias_container.ptr);
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
