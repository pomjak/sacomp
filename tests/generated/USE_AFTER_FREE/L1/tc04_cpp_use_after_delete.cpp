/* === MUTATOR: L1_interprocedural ===
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
/* L1: defect moved into helper — inter-procedural boundary */
static void cpp_use_after_delete_bad_impl()
{
    Node *obj = new Node(99);
    delete obj;

    /* BUG: obj is deleted; virtual dispatch / vtable read is UB */
    obj->print();
}


void cpp_use_after_delete_bad(void)
{
    cpp_use_after_delete_bad_impl();
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
