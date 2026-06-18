# mutator — Clang LibTooling Test Case Complexity Mutator

## What it does

`mutator` reads C++ test case files that contain a `_bad()` function encoding
a defect, and emits **6 complexity variants** (L0–L5) for each file:

| Level | Name | Transformation applied to `_bad()` body |
|-------|------|------------------------------------------|
| L0 | `base` | Verbatim copy — no change |
| L1 | `interprocedural` | Defect body moved into a `_bad_impl()` helper; `_bad()` becomes a one-line forwarder |
| L2 | `aliasing` | First pointer variable re-routed through a `struct AliasContainer { T *ptr; }` field; all subsequent uses replaced with `_alias_container.ptr` |
| L3 | `opaque_branch` | Entire body wrapped in `if (getenv("MUTATOR_TRIGGER") != nullptr)` — condition is always true at runtime but opaque to static analyzers |
| L4 | `loop_wrap` | Body wrapped in `for (_i = 0; _i < 1; ++_i)` — stresses loop-aware reasoning |
| L5 | `combined` | L2 → L1 → L3 applied in sequence; deepest transformation |

Output directory layout:

```
mutated/
├── L0/   CWE401_MemLeak/  tc01_simple_malloc_no_free.cpp  ...
├── L1/   ...
├── L2/   ...
├── L3/   ...
├── L4/   ...
└── L5/   ...
```

---

## Prerequisites — Arch Linux (pacman + AUR/yay)

### 1. Install base development tools

```bash
sudo pacman -S --needed base-devel git cmake ninja
```

### 2. Install LLVM and Clang with development headers

The key package is `clang` which on Arch pulls in `llvm` and ships
the LibTooling headers and CMake modules:

```bash
sudo pacman -S --needed llvm clang
```

This installs:
- `/usr/lib/cmake/llvm/`   — `LLVMConfig.cmake`
- `/usr/lib/cmake/clang/`  — `ClangConfig.cmake`
- `/usr/include/clang/`    — all Clang headers including LibTooling
- `/usr/lib/libclang*.a`   — static libraries

Verify the versions match:

```bash
llvm-config --version   # e.g. 18.1.x
clang --version
```

> **Version note**: LLVM/Clang on Arch tracks the latest stable release.
> The CMakeLists.txt in this repo uses `find_package(LLVM)` and
> `find_package(Clang)` without a pinned version, so it works with
> whatever Arch ships.

### 3. (Optional) Install from AUR for bleeding-edge LLVM

If you need a specific LLVM version not yet in the official repos:

```bash
yay -S llvm-git clang-git   # latest git snapshot — may take ~1 h to build
```

Or a pinned older version:

```bash
yay -S llvm17 clang17        # example: LLVM 17 from AUR
```

When using an AUR version you may need to point CMake at it explicitly:

```bash
cmake \
  -DLLVM_DIR=/usr/lib/llvm17/lib/cmake/llvm \
  -DClang_DIR=/usr/lib/llvm17/lib/cmake/clang \
  ..
```

### 4. Install the static analyzer tools (for running experiments)

```bash
sudo pacman -S --needed cppcheck
# clang-tools-extra ships scan-build (Clang Static Analyzer wrapper)
sudo pacman -S --needed clang-tools-extra
# Infer is AUR-only on Arch:
yay -S infer-bin             # pre-built binary — much faster than building from source
```

---

## Building the mutator

```bash
# 1. Clone / copy the mutator directory
cd /path/to/project
ls
#  mutator.cpp   CMakeLists.txt   run_mutator.sh   README_mutator.md

# 2. Create build directory
mkdir build && cd build

# 3. Configure
cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# If cmake cannot find LLVM/Clang automatically:
cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_DIR=/usr/lib/cmake/llvm \
      -DClang_DIR=/usr/lib/cmake/clang \
      ..

# 4. Build
ninja

# Binary appears at: ./build/mutator
```

### Expected cmake output (success looks like)

```
-- LLVM version : 18.1.6
-- LLVM headers : /usr/include
-- Clang headers: /usr/include
-- Configuring done
-- Build files have been written to: .../build
```

### Common cmake errors and fixes

| Error | Fix |
|-------|-----|
| `Could not find a package configuration file provided by "LLVM"` | `sudo pacman -S llvm` then re-run cmake |
| `Could not find a package configuration file provided by "Clang"` | `sudo pacman -S clang` |
| `error: unknown type name '__uint128_t'` | Add `-DCMAKE_CXX_FLAGS="-fno-rtti"` |
| Link error: `undefined reference to clang::tooling::...` | Ensure `clang` (not just `clang-libs`) is installed; the static libs may be in a separate package — try `sudo pacman -S clang` |
| `ninja: error: build.ninja: No such file` | You forgot `cmake ..` before `ninja` |

---

## Usage

### Basic — process all 25 test cases

```bash
# From the project root (mutator/build/mutator exists, tests/ directory exists)
chmod +x run_mutator.sh
./run_mutator.sh ./build/mutator ./tests ./mutated
```

### Manual — single file, specific levels

```bash
./build/mutator \
    --levels=1,2,3 \
    --outdir=./mutated \
    tests/CWE401_MemLeak/tc01_simple_malloc_no_free.cpp \
    -- -std=c++14
```

The `--` separates mutator flags from compiler flags forwarded to Clang.
Always include `-std=c++14` (or c++17) to ensure correct parsing.

### All levels on all files (one-liner)

```bash
./build/mutator \
    --levels=0,1,2,3,4,5 \
    --outdir=./mutated \
    $(find tests -name "*.cpp" | sort) \
    -- -std=c++14
```

---

## Running the static analyzers on mutated output

### Cppcheck

```bash
for lvl in 0 1 2 3 4 5; do
    echo "=== L${lvl} ==="
    cppcheck --enable=all --std=c++14 --xml \
             mutated/L${lvl}/ 2> results/cppcheck_L${lvl}.xml
done
```

### Clang Static Analyzer (scan-build)

```bash
mkdir -p results/csa
for lvl in 0 1 2 3 4 5; do
    echo "=== L${lvl} ==="
    scan-build -o results/csa/L${lvl} \
        clang++ -std=c++14 \
        mutated/L${lvl}/*/*.cpp -c 2>&1 \
        | tee results/csa_L${lvl}.txt
done
```

### Facebook Infer

```bash
mkdir -p results/infer
for lvl in 0 1 2 3 4 5; do
    echo "=== L${lvl} ==="
    infer run --results-dir results/infer/L${lvl} \
         -- clang++ -std=c++14 \
         mutated/L${lvl}/*/*.cpp -c
done
# JSON reports in results/infer/L*/report.json
```

---

## How the tool works internally

```
 ┌──────────────────────────────────────────────────────────┐
 │  main()                                                  │
 │   parse --levels, --outdir via llvm::cl                  │
 │   build ClangTool with CommonOptionsParser               │
 │   run MutatorActionFactory on each input file            │
 └──────────────────┬───────────────────────────────────────┘
                    │  per translation unit
                    ▼
 ┌──────────────────────────────────────────────────────────┐
 │  MutatorAction  (ASTFrontendAction)                      │
 │   creates MutatorConsumer                                │
 └──────────────────┬───────────────────────────────────────┘
                    │
                    ▼
 ┌──────────────────────────────────────────────────────────┐
 │  MutatorConsumer  (ASTConsumer)                          │
 │   HandleTranslationUnit():                               │
 │     1. Run DefectVisitor → fill BadFuncInfo              │
 │     2. Read source file as plain text                    │
 │     3. For each requested level call emitLN()            │
 │     4. Write transformed text to outdir/LN/filename.cpp  │
 └──────────────────┬───────────────────────────────────────┘
                    │
                    ▼
 ┌──────────────────────────────────────────────────────────┐
 │  DefectVisitor  (RecursiveASTVisitor<DefectVisitor>)     │
 │   VisitFunctionDecl():                                   │
 │     find first FunctionDecl whose name ends in "_bad"   │
 │     record: name, return type, param list, body text,   │
 │             byte offsets of body in source file          │
 └──────────────────────────────────────────────────────────┘

 Text-level emitters (no second AST pass needed):
   emitL0 — verbatim copy
   emitL1 — inject helper before function, replace body with call
   emitL2 — scan body for first pointer decl, add alias struct,
             rename uses after declaration line
   emitL3 — strip outer braces, re-wrap in if(getenv(...))
   emitL4 — strip outer braces, re-wrap in for(i<1)
   emitL5 — chain: emitL2 → re-locate body in result → emitL1
             → re-locate forwarder body → emitL3
```

### Why text-level emitters instead of Rewriter?

`clang::Rewriter` works beautifully for small, local substitutions but
becomes complex when you need to *inject new top-level declarations*
(helper functions) before an existing one, because you must track
`FileID` and absolute file offsets.  For the transforms we apply here
(whole-body replacement + optional function injection) string splicing
at recorded byte offsets is simpler and equally correct — the AST is
used only to *locate* the body, not to walk its internal structure.

---

## File layout

```
mutator/
├── mutator.cpp          main source
├── CMakeLists.txt       build system
├── run_mutator.sh       convenience batch script
└── README_mutator.md    this file
```

---

## Troubleshooting

### `error: 'make_unique' is not a member of 'std'`
Add `-DCMAKE_CXX_STANDARD=17` to your cmake invocation (C++14 lacks
`std::make_unique`; the tool uses C++17 minimum).

### Mutator prints `No _bad() function found`
The tool looks for a function whose name ends in exactly `_bad`.
Check that your test file follows the naming convention used in the
base test cases (e.g., `double_free_bad`, `loop_uaf_bad`).

### L2 output says `alias transform skipped — no pointer decl found`
The heuristic scans for lines containing `malloc(`, `new `, or `new[`.
Pure C++ tests that allocate via constructors only may not match.
You can pre-process the file to use explicit `new`/`malloc` calls, or
extend `emitL2` with additional heuristics.

### Segfault / abort when running mutated L3/L5 files manually
L3 wraps the defect in `if (getenv("MUTATOR_TRIGGER") != nullptr)`.
At runtime this is true only when the env variable is set:
```bash
MUTATOR_TRIGGER=1 ./a.out
```
Without the env var, `_bad()` body does not execute and the program
exits cleanly — which is intentional for compilation/analysis testing.

### Infer reports no bugs on L3 files
This is the expected result demonstrating L3's effectiveness as a
stress test: Infer (and most other analyzers) cannot resolve
`getenv()` return values statically and conservatively skip that branch.
Document it in your results table.
