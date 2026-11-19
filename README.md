# SACOMP

SACOMP is a benchmark for comparing memory-safety findings reported by
**Cppcheck**, **Clang Static Analyzer**, and **Facebook Infer** on C++ test
cases. It runs each analyzer, normalizes their different diagnostic formats
into common categories, calculates precision/recall/F1 metrics, and generates
comparison plots.

The project focuses on:

- memory leaks
- double free
- use-after-free
- null pointer dereferences
- uninitialized values

The accompanying evaluation report is available in
[`docs/Comparative_Evaluation_of_Static_Analysis_Tools.pdf`](docs/Comparative_Evaluation_of_Static_Analysis_Tools.pdf).

## Repository layout

```text
.
├── README.md
├── src
│   ├── main.py          # benchmark entry point
│   ├── tools.py         # analyzer wrappers
│   ├── parser.py        # diagnostic parsing and normalization
│   ├── metrics.py       # TP/FP/FN and precision/recall/F1
│   ├── graphs.py        # benchmark plots
│   └── requirements.txt
├── tests
│   └── mutator
│       ├── mutator.py
│       └── run_mutator.sh
└── docs
    ├── Comparative_Evaluation_of_Static_Analysis_Tools.pdf
    ├── benchmark_results.png
    └── main.tex
```

Test cases are expected to be organized as:

```text
<test-root>/
└── <category>/
    └── <level>/
        └── *.cpp
```

The category directory supplies the expected vulnerability label and the level
directory supplies the complexity level used in the per-level statistics.
For example, `tests/generated/USE_AFTER_FREE/L2/example.cpp` is evaluated as a
level-2 use-after-free test.

## Requirements

Install Python dependencies:

```bash
python3 -m pip install -r src/requirements.txt
```

The benchmark also requires these commands to be available on `PATH`:

- `cppcheck`
- `clang`
- `clang++`
- `infer`

The mutator additionally requires Python `libclang` bindings and a compatible
Clang installation.

## Running the benchmark

From the repository root, pass the directory containing the C++ test cases:

```bash
python3 src/main.py <test-root>
```

For example:

```bash
python3 src/main.py tests/generated
```

The runner recursively finds every `.cpp` file and analyzes it with all three
tools. It prints overall, category-level, and category/complexity-level
statistics, then writes `benchmark_results.png` to the current working
directory. Temporary Infer output and object files are removed after the run.

## Generating mutated test cases

`tests/mutator/mutator.py` uses `libclang` to find the vulnerable `_bad()`
function in a seed file and generate increasingly complex variants. The
available levels are:

| Level | Transformation |
| --- | --- |
| L0 | Original seed |
| L1 | Interprocedural helper-function boundary |
| L2 | Pointer aliasing through a structure field |
| L3 | Runtime-dependent opaque branch |
| L4 | Loop around the vulnerable operation |
| L5 | Combination of the previous transformations |

Run the supplied wrapper from `tests/mutator`:

```bash
cd tests/mutator
./run_mutator.sh <mutator> <input-directory> <category-output-directory>
```

The mutator writes files as `<category-output-directory>/L0` through `L5`.
Choose an output directory named after the expected category to produce the
layout consumed by the benchmark. For example:

```bash
cd ../..
python3 tests/mutator/mutator.py \
  --levels=0,1,2,3,4,5 \
  --outdir=tests/generated/USE_AFTER_FREE \
  tests/seeds/<category>/seed.cpp \
  -- -std=c++14
```

The input seed must contain a function whose name ends in `_bad()`. Generated
files are placed under the selected category and level directories so that the
benchmark can infer their expected labels.

## Evaluation model

Analyzer-specific diagnostics are converted to a shared `Finding` format in
`src/parser.py`. For each test file:

- a true positive means the expected vulnerability category was reported;
- a false negative means the expected category was not reported;
- a false positive means an additional, different category was reported.

The benchmark reports precision, recall, and F1 overall and grouped by
vulnerability category and mutation level. Analyzer output is treated as
diagnostic data; the benchmark does not execute the test programs.

## Notes

- The Infer wrapper expects its report at `infer-out/report.json`.
- Clang Static Analyzer reports are collected in plist format.
- Cppcheck is invoked with XML output.
- Tool versions and analyzer configuration can affect results, so record them
  when comparing benchmark runs.
