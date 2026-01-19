#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 8.3 Validation: Common Subexpression Elimination ==="

# Check that the test file exists
[ -f "tests/test_cse.wyn" ] || { echo "ERROR: test_cse.wyn not found"; exit 1; }

# Check that compiler exists
[ -f "wyn" ] || { echo "ERROR: wyn compiler not found"; exit 1; }

# Compile the test
echo "Compiling test_cse.wyn..."
./wyn tests/test_cse.wyn || { echo "ERROR: Compilation failed"; exit 1; }

# Check that output was generated
[ -f "tests/test_cse.wyn.out" ] || { echo "ERROR: Output executable not generated"; exit 1; }

# Run the test
echo "Running CSE test..."
./tests/test_cse.wyn.out || { echo "ERROR: Test execution failed"; exit 1; }

echo "✓ Task 8.3 Complete - Common Subexpression Elimination"
exit 0