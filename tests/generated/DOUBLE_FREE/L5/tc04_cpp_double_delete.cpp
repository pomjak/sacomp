/* === MUTATOR: L5_combined ===
 * Source  : ./DOUBLE_FREE/tc04_cpp_double_delete.cpp
 * Function: cpp_double_delete_bad
 * ======================================================= */
/*
 * CWE-415: Double Free
 * TC04 - C++ delete called twice on the same object pointer.
 *        Tests whether analyzers handle C++ deallocation operators.
 *
 * BAD:  delete obj called twice.
 * GOOD: obj set to nullptr after first delete.
 */
#include <cstdlib> /* L3: for getenv */
#include <cstdio>

struct Widget {
    int value;
    Widget(int v) : value(v) {}
    ~Widget() { printf("Widget %d destroyed\n", value); }
};

/* BAD */
/* L1: defect moved into helper — inter-procedural boundary */
static void cpp_double_delete_bad_impl()
{
    /* L2: alias introduced through struct field */
    struct AliasContainer {
        Widget *ptr = nullptr;
    } _alias_container;

    Widget *obj = new Widget(42);
    _alias_container.ptr = obj;
    printf("Widget value: %d\n", _alias_container.ptr->value);

    delete _alias_container.ptr;    /* first delete */
    delete _alias_container.ptr;    /* BUG: second delete — undefined behaviour */
}

void cpp_double_delete_bad(void)
{
    /* L3: defect wrapped in opaque runtime condition */
    /* Set env var MUTATOR_TRIGGER=1 to activate at runtime. */
    if (__builtin_expect(getenv("MUTATOR_TRIGGER") != nullptr, 1))
    {

    cpp_double_delete_bad_impl();

    }
}

/* GOOD */
void cpp_double_delete_good(void)
{
    Widget *obj = new Widget(42);
    printf("Widget value: %d\n", obj->value);

    delete obj;
    obj = nullptr;   /* safe: delete nullptr is well-defined no-op */
    delete obj;
}

int main(void)
{
    cpp_double_delete_good();
    cpp_double_delete_bad();
    return 0;
}
