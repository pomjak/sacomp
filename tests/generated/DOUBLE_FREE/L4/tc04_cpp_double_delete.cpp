/* === MUTATOR: L4_loop_wrap ===
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
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    Widget *obj = new Widget(42);
    printf("Widget value: %d\n", obj->value);

    delete obj;    /* first delete */
    delete obj;    /* BUG: second delete — undefined behaviour */

    }
}

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
