/* === MUTATOR: L4_loop_wrap ===
 * Source  : ./USE_AFTER_FREE/tc04_cpp_use_after_delete.cpp
 * Function: cpp_use_after_delete_bad
 * ======================================================= */
/*
 * CWE-416: Use After Free
 * TC04 - C++ object is deleted and then a member function is called
 *        through the now-dangling pointer.
 *
 * BAD:  obj->print() called after delete obj.
 * GOOD: obj->print() called before delete obj.
 */
#include <cstdio>

struct Node {
    int value;
    explicit Node(int v) : value(v) {}
    void print() const { printf("Node value: %d\n", value); }
};

/* BAD */
void cpp_use_after_delete_bad(void)
{
    /* L4: defect wrapped in loop — tests loop-aware reasoning */
    const int _loop_iters = 1;
    for (int _i = 0; _i < _loop_iters; ++_i)
    {

    Node *obj = new Node(99);
    delete obj;

    /* BUG: obj is deleted; virtual dispatch / vtable read is UB */
    obj->print();

    }
}
/* GOOD */
void cpp_use_after_delete_good(void)
{
    Node *obj = new Node(99);
    obj->print();   /* use before delete */
    delete obj;
    obj = nullptr;
}

int main(void)
{
    cpp_use_after_delete_good();
    cpp_use_after_delete_bad();
    return 0;
}
