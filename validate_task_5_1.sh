#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

# Should fail to compile
if ./wyn tests/test_exhaustiveness_fail.wyn 2>&1 | grep -q "non-exhaustive"; then
    echo "✓ Exhaustiveness checking works"
else
    echo "✗ Exhaustiveness checking failed"
    exit 1
fi

# Should compile with wildcard
./wyn tests/test_exhaustiveness_ok.wyn || exit 1
echo "✓ Task 5.1 Complete"