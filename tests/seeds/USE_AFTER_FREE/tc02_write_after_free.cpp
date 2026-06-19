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
    char *buf = (char *)malloc(64);
    if (buf == NULL) return;

    strcpy(buf, "initial");
    printf("before free: %s\n", buf);
    free(buf);

    /* BUG: writing to freed memory */
    strcpy(buf, src);
    printf("after free: %s\n", buf);
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
