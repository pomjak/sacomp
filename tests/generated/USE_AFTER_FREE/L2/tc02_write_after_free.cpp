/* === MUTATOR: L2_aliasing ===
 * Source  : ./USE_AFTER_FREE/tc02_write_after_free.cpp
 * Function: write_after_free_bad
 * ======================================================= */
/*
 * CWE-416: Use After Free
 * TC02 - Write to a heap buffer after it has been freed.
 *        Write-after-free is particularly dangerous as it may corrupt
 *        allocator metadata or adjacent objects.
 *
 * BAD:  buf is freed, then a value is written through it.
 * GOOD: write happens before free.
 */
#include <cstdlib>
#include <cstring>
#include <cstdio>

/* BAD */
void write_after_free_bad(const char *src)
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *buf = (char *)malloc(64);
    _alias_container.ptr = buf;
    if (_alias_container.ptr == NULL) return;

    strcpy(_alias_container.ptr, "initial");
    printf("before free: %s\n", _alias_container.ptr);
    free(_alias_container.ptr);

    /* BUG: writing to freed memory */
    strcpy(_alias_container.ptr, src);
    printf("after free: %s\n", _alias_container.ptr);
}

/* GOOD */
void write_after_free_good(const char *src)
{
    char *buf = (char *)malloc(64);
    if (buf == NULL) return;

    strcpy(buf, "initial");
    printf("before: %s\n", buf);
    strcpy(buf, src);          /* write before free */
    printf("after update: %s\n", buf);
    free(buf);
}

int main(void)
{
    write_after_free_good("updated");
    write_after_free_bad("updated");
    return 0;
}
