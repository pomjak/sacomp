/* === MUTATOR: L2_aliasing ===
 * Source  : ./MEMORY_LEAK/tc02_leak_on_error_path.cpp
 * Function: load_config_bad
 * ======================================================= */
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
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        char *ptr = nullptr;
    } _alias_container;

    char *copy = (char *)malloc(256);
    _alias_container.ptr = copy;
    if (_alias_container.ptr == NULL)
        return -1;

    strncpy(_alias_container.ptr, input, 255);
    _alias_container.ptr[255] = '\0';

    if (!validate(_alias_container.ptr)) {
        /* BUG: returns without freeing _alias_container.ptr */
        return -2;
    }

    printf("config: %s\n", _alias_container.ptr);
    free(_alias_container.ptr);
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
