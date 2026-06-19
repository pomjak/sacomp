/* === MUTATOR: L2_aliasing ===
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
#include <cstdio>

struct Widget {
    int value;
    Widget(int v) : value(v) {}
    ~Widget() { printf("Widget %d destroyed\n", value); }
};

/* BAD */
void cpp_double_delete_bad(void)
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
