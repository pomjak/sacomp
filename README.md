# sacomp
comparative analysis of static analyzators (Infer, cppcheck, clang sa), testing with picked and mutated tests from NIST JULIET test set of cpp programs 


## overview of structure
```text
.
├── main.py             # entry point
├── README.md           # this file
├── src
│   ├── generate.py     # generation/mutation of tests
│   ├── metrics.py      # statistics/comparisons
│   ├── parser.py       # parsing/normalization
│   └── tools.py        # wrapper for tools
└── tests
    ├── generated       # generated/mutated test sets
    └── seeds           # manually crafted or picked tests
```

