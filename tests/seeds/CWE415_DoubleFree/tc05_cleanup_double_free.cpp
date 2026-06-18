/*
 * CWE-415: Double Free
 * TC05 - A common real-world pattern: a cleanup label / goto is used
 *        for error handling, but the buffer is also freed on the
 *        success path, so taking the error path after success frees twice.
 *
 *        Simplified here as two sequential cleanup calls.
 *
 * BAD:  cleanup() frees buf; caller also frees buf.
 * GOOD: cleanup() nullifies caller's pointer via double-pointer.
 */
#include <cstdlib>
#include <cstdio>

/* BAD: consumes the pointer without telling the caller */
static void cleanup_bad(char *buf)
{
    if (buf) free(buf);
}

/* GOOD: nulls out the caller's pointer */
static void cleanup_good(char **buf)
{
    if (buf && *buf) {
        free(*buf);
        *buf = NULL;
    }
}

/* BAD scenario */
void process_bad(void)
{
    char *buf = (char *)malloc(256);
    if (buf == NULL) return;

    buf[0] = 'Z';

    int err = (buf[0] != 'Z');  /* always 0, but analyzer doesn't know */
    if (err) {
        cleanup_bad(buf);
        return;
    }

    cleanup_bad(buf);   /* first free via cleanup */
    free(buf);          /* BUG: second free */
}

/* GOOD scenario */
void process_good(void)
{
    char *buf = (char *)malloc(256);
    if (buf == NULL) return;

    buf[0] = 'Z';

    int err = (buf[0] != 'Z');
    if (err) {
        cleanup_good(&buf);
        return;
    }

    cleanup_good(&buf);   /* buf is now NULL */
    free(buf);            /* safe: free(NULL) */
}

int main(void)
{
    process_good();
    /* process_bad(); */
    return 0;
}
