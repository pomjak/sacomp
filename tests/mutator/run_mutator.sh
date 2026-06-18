#!/usr/bin/env bash
# run_mutator.sh
#
# Runs the compiled mutator on all 25 base test cases and places output
# in ./mutated/{L0..L5}/.
#
# Usage:
#   ./run_mutator.sh [path/to/mutator] [path/to/tests] [outdir]
#
# Defaults:
#   mutator binary : ./build/mutator
#   test root      : ./tests
#   output dir     : ./mutated

set -euo pipefail

MUTATOR="${1:-./build/mutator}"
TEST_ROOT="${2:-./tests}"
OUT_DIR="${3:-./mutated}"
LEVELS="0,1,2,3,4,5"

if [[ ! -x "$MUTATOR" ]]; then
    echo "[ERROR] Mutator binary not found at: $MUTATOR"
    echo "        Build it first (see README_mutator.md)."
    exit 1
fi

if [[ ! -d "$TEST_ROOT" ]]; then
    echo "[ERROR] Test root directory not found: $TEST_ROOT"
    exit 1
fi

echo "=== Mutator run ==="
echo "  binary   : $MUTATOR"
echo "  test root: $TEST_ROOT"
echo "  output   : $OUT_DIR"
echo "  levels   : $LEVELS"
echo ""

# Collect all .cpp files
mapfile -t CPP_FILES < <(find "$TEST_ROOT" -name "*.cpp" | sort)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
    echo "[ERROR] No .cpp files found under $TEST_ROOT"
    exit 1
fi

echo "Found ${#CPP_FILES[@]} source files."
echo ""

# Run mutator on each file individually so failures don't abort the whole run
PASS=0
FAIL=0
for f in "${CPP_FILES[@]}"; do
    echo "Processing: $f"
    if "$MUTATOR" \
            --levels="$LEVELS" \
            --outdir="$OUT_DIR" \
            "$f" \
            -- -std=c++14 2>&1; then
        PASS=$((PASS + 1))
    else
        echo "[WARN] Mutator returned non-zero for $f"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "=== Done ==="
echo "  Processed : $((PASS + FAIL)) files"
echo "  OK        : $PASS"
echo "  Warnings  : $FAIL"
echo ""
echo "Output layout:"
find "$OUT_DIR" -name "*.cpp" | sort | head -40
