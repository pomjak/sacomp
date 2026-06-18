/*
 * CWE-401: Memory Leak
 * TC02 - Leak on error branch: free is present on the happy path
 *        but omitted when validation fails.
 *
 * BAD:  early return on validation error skips free().
 * GOOD: free() called before every return.
 */
#include <cstdlib>
#include <cstring>
#include <cstdio>

static int validate(const char *s) { return s[0] != '\0'; }

/* BAD */
int load_config_bad(const char *input)
{
    char *copy = (char *)malloc(256);
    if (copy == NULL)
        return -1;

    strncpy(copy, input, 255);
    copy[255] = '\0';

    if (!validate(copy)) {
        /* BUG: returns without freeing copy */
        return -2;
    }

    printf("config: %s\n", copy);
    free(copy);
    return 0;
}

/* GOOD */
int load_config_good(const char *input)
{
    char *copy = (char *)malloc(256);
    if (copy == NULL)
        return -1;

    strncpy(copy, input, 255);
    copy[255] = '\0';

    if (!validate(copy)) {
        free(copy);   /* fix: free before early return */
        return -2;
    }

    printf("config: %s\n", copy);
    free(copy);
    return 0;
}

int main(void)
{
    load_config_bad("");
    load_config_good("");
    return 0;
}
