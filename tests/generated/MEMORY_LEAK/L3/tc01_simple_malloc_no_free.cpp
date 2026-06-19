/* === MUTATOR: L3_opaque_branch ===
 * Source  : ./MEMORY_LEAK/tc01_simple_malloc_no_free.cpp
 * Function: process_data_bad
 * ======================================================= */
/*
 * CWE-401: Memory Leak
 * TC01 - Simple malloc with no free on any path.
 *
 * BAD:  process_data() allocates a buffer and returns without freeing it.
 * GOOD: process_data_good() frees the buffer before returning.
 */
#include <cstdlib>
#include <cstring>
#include <cstdio>

/* BAD: buffer is never freed */
void process_data_bad(int size)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    char *buf = (char *)malloc(size);
    if (buf == NULL)
        return;

    memset(buf, 0, size);
    printf("processed %d bytes\n", size);
    /* missing: free(buf); */

    }
}
/* GOOD: buffer is freed before return */
void process_data_good(int size)
{
    char *buf = (char *)malloc(size);
    if (buf == NULL)
        return;

    memset(buf, 0, size);
    printf("processed %d bytes\n", size);
    free(buf);
}

int main(void)
{
    process_data_bad(64);
    process_data_good(64);
    return 0;
}
