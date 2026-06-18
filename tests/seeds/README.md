# Base Test Cases — CWE-401 / 415 / 416 / 457 / 476

25 single-file C++ test cases, 5 per CWE, at baseline (simple) complexity.
Each file contains a `_bad` function (the defective variant) and a `_good`
function (the correct counterpart).  The `_bad` call in `main()` is commented
out so all 25 files compile and run cleanly without crashing.

---

## Directory layout

```
tests/
├── CWE401_MemLeak/
│   ├── tc01_simple_malloc_no_free.cpp      malloc never freed
│   ├── tc02_leak_on_error_path.cpp         free missing on error branch
│   ├── tc03_new_array_no_delete.cpp        new[] without delete[]
│   ├── tc04_pointer_overwrite_leak.cpp     pointer reassigned before free
│   └── tc05_struct_inner_leak.cpp          heap field in struct leaked
├── CWE415_DoubleFree/
│   ├── tc01_simple_double_free.cpp         free() called twice directly
│   ├── tc02_conditional_double_free.cpp    free in branch + unconditional free
│   ├── tc03_alias_double_free.cpp          two pointers to same block both freed
│   ├── tc04_cpp_double_delete.cpp          C++ delete called twice
│   └── tc05_cleanup_double_free.cpp        cleanup helper + caller both free
├── CWE416_UseAfterFree/
│   ├── tc01_simple_read_after_free.cpp     read after free
│   ├── tc02_write_after_free.cpp           write after free
│   ├── tc03_dangling_alias.cpp             alias frees block; original used
│   ├── tc04_cpp_use_after_delete.cpp       method called on deleted C++ object
│   └── tc05_loop_use_after_free.cpp        freed at end of iteration; used next
├── CWE457_UninitVar/
│   ├── tc01_simple_uninit_read.cpp         local int read before assignment
│   ├── tc02_branch_uninit.cpp              assigned only in one branch
│   ├── tc03_struct_partial_init.cpp        struct field left uninitialized
│   ├── tc04_uninit_pointer.cpp             uninitialized pointer passed to sink
│   └── tc05_uninit_array_index.cpp         uninitialized int used as array index
└── CWE476_NullDeref/
    ├── tc01_malloc_no_null_check.cpp       malloc return used without check
    ├── tc02_deref_before_null_check.cpp    deref happens before the null check
    ├── tc03_null_return_no_check.cpp       function return value not checked
    ├── tc04_null_after_reset.cpp           ptr set to NULL in branch, used after
    └── tc05_chained_null_deref.cpp         intermediate pointer in chain is NULL
```

---

## Running the analyzers

### Cppcheck
```bash
# Run on a single file
cppcheck --enable=all --std=c++14 \
         CWE401_MemLeak/tc01_simple_malloc_no_free.cpp

# Run on all files, machine-readable XML output
cppcheck --enable=all --std=c++14 --xml \
         CWE401_MemLeak/ CWE415_DoubleFree/ \
         CWE416_UseAfterFree/ CWE457_UninitVar/ CWE476_NullDeref/ \
         2> cppcheck_results.xml
```

### Clang Static Analyzer
```bash
# Single file
clang --analyze -std=c++14 \
      -Xanalyzer -analyzer-checker=alpha.security.MallocOverflow \
      -Xanalyzer -analyzer-checker=alpha.unix.MallocWithAnnotations \
      -Xanalyzer -analyzer-checker=unix.Malloc \
      -Xanalyzer -analyzer-checker=alpha.cplusplus \
      -Xanalyzer -analyzer-checker=core \
      CWE401_MemLeak/tc01_simple_malloc_no_free.cpp

# Whole directory via scan-build
scan-build -o ./csa_reports clang++ -std=c++14 \
    CWE401_MemLeak/*.cpp CWE415_DoubleFree/*.cpp \
    CWE416_UseAfterFree/*.cpp CWE457_UninitVar/*.cpp \
    CWE476_NullDeref/*.cpp
```

### Facebook Infer
```bash
# Capture + analyse a single file
infer run -- clang++ -std=c++14 \
    CWE401_MemLeak/tc01_simple_malloc_no_free.cpp

# Capture all files in one infer run
infer run -- clang++ -std=c++14 \
    CWE401_MemLeak/*.cpp CWE415_DoubleFree/*.cpp \
    CWE416_UseAfterFree/*.cpp CWE457_UninitVar/*.cpp \
    CWE476_NullDeref/*.cpp

# Results in infer-out/report.json
```

---

## Expected detection notes

| TC | Cppcheck | CSA | Infer | Notes |
|----|----------|-----|-------|-------|
| CWE401-tc01 | ✓ | ✓ | ✓ | Trivial; all tools should catch |
| CWE401-tc02 | ~ | ✓ | ✓ | Path-sensitive tools catch error-path leak |
| CWE401-tc03 | ✓ | ✓ | ~ | C++ new[]; Infer C++ support varies |
| CWE401-tc04 | ✓ | ✓ | ✓ | Pointer overwrite |
| CWE401-tc05 | ~ | ~ | ✓ | Nested alloc; Infer separates logic strongest |
| CWE415-tc01 | ✓ | ✓ | ✓ | Trivial double free |
| CWE415-tc02 | ~ | ✓ | ✓ | Needs path sensitivity |
| CWE415-tc03 | ~ | ✓ | ✓ | Alias; CSA tracks MemRegion |
| CWE415-tc04 | ~ | ✓ | ~ | C++ delete; coverage varies |
| CWE415-tc05 | ~ | ~ | ✓ | Cross-function; Infer compositional |
| CWE416-tc01 | ✓ | ✓ | ✓ | Trivial UAF |
| CWE416-tc02 | ✓ | ✓ | ✓ | Write UAF |
| CWE416-tc03 | ~ | ✓ | ✓ | Alias-based UAF |
| CWE416-tc04 | ~ | ✓ | ~ | C++ method on deleted object |
| CWE416-tc05 | ~ | ✓ | ~ | Loop-based UAF |
| CWE457-tc01 | ✓ | ✓ | ~ | Simple uninit; Infer focuses on memory |
| CWE457-tc02 | ✓ | ✓ | ~ | Branch uninit |
| CWE457-tc03 | ✓ | ✓ | ~ | Struct field uninit |
| CWE457-tc04 | ~ | ✓ | ~ | Uninit pointer; overlaps UAF checkers |
| CWE457-tc05 | ✓ | ~ | ~ | Uninit index; CSA may not flag modulo use |
| CWE476-tc01 | ✓ | ✓ | ✓ | malloc without null check |
| CWE476-tc02 | ~ | ✓ | ~ | Deref before check; path-sensitive |
| CWE476-tc03 | ~ | ✓ | ✓ | Null return propagation |
| CWE476-tc04 | ~ | ✓ | ✓ | Pointer reset to null in branch |
| CWE476-tc05 | ~ | ✓ | ✓ | Chained null in struct field |

Legend: ✓ likely detected · ~ uncertain / tool-dependent · verify experimentally

---

## Mutation plan (next layer)

For each base TC, the following transformations will produce mutated variants:

1. **M1 — inter-procedural split**: move the defect across a function boundary
   (alloc in caller, bug in callee, or vice versa).
2. **M2 — opaque branch**: wrap the defect path in a branch whose condition
   depends on a runtime value (`argc`, `getenv`) that the analyzer cannot
   resolve statically.
3. **M3 — aliasing via struct field**: route the defective pointer through
   a struct field rather than a direct local variable.
4. **M4 — loop wrapping**: place the defect inside a loop body so the tool
   must reason about loop iterations.
