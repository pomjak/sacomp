#!/usr/bin/env bash
# run_mutator.sh

set -euo pipefail

DIR="UNINITIALIZED_VALUE"
MUTATOR="${1:-./mutator.py}"
TEST_ROOT="${2:-./$DIR}"
OUT_DIR="${3:-./mutated-$DIR}"
LEVELS="0,1,2,3,4,5"

if [[ ! -f "$MUTATOR" ]]; then
    echo "[ERROR] Mutator script not found at: $MUTATOR"
    echo "        Make sure mutator.py is in the current directory."
    exit 1
fi

if [[ ! -d "$TEST_ROOT" ]]; then
    echo "[ERROR] Test root directory not found: $TEST_ROOT"
    exit 1
fi

echo "=== Mutator run ==="
echo "  script   : $MUTATOR"
echo "  test root: $TEST_ROOT"
echo "  output   : $OUT_DIR"
echo "  levels   : $LEVELS"
echo ""

mapfile -t CPP_FILES < <(find "$TEST_ROOT" -name "*.cpp" | sort)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
    echo "[ERROR] No .cpp files found under $TEST_ROOT"
    exit 1
fi

echo "Found ${#CPP_FILES[@]} source files."
echo ""

PASS=0
FAIL=0
for f in "${CPP_FILES[@]}"; do
    echo "Processing: $f"
    
    if python3 "$MUTATOR" \
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