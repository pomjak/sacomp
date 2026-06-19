/* === MUTATOR: L5_combined ===
 * Source  : ./UNINITIALIZED_VALUE/tc03_struct_partial_init.cpp
 * Function: distance_from_origin_bad
 * ======================================================= */
/*
 * CWE-457: Use of Uninitialized Variable
 * TC03 - A struct is partially initialized: one field is set explicitly
 *        but another is left uninitialized, and the uninitialized field
 *        is subsequently used.
 *
 * BAD:  point.y is never set; used in distance calculation.
 * GOOD: both fields initialized before use.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>
#include <cmath>

struct Point {
    double x;
    double y;
};

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static double distance_from_origin_bad_impl()
{
    /* L2: alias transform skipped — no pointer decl found */

    Point p;
    p.x = 3.0;
    /* BUG: p.y is uninitialized */
    double dist = sqrt(p.x * p.x + p.y * p.y);
    printf("distance = %f\n", dist);
    return dist;
}

double distance_from_origin_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    return distance_from_origin_bad_impl();

    }
    return 0.0;
}

/* GOOD */
double distance_from_origin_good(void)
{
    Point p;
    p.x = 3.0;
    p.y = 4.0;   /* initialized */
    double dist = sqrt(p.x * p.x + p.y * p.y);
    printf("distance = %f\n", dist);
    return dist;
}

int main(void)
{
    distance_from_origin_good();
    distance_from_origin_bad();
    return 0;
}
