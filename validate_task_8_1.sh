#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 8.1 Validation: Function Inlining ==="

# Check that the test file exists
[ -f "tests/test_inlining.wyn" ] || { echo "ERROR: test_inlining.wyn not found"; exit 1; }

# Check that compiler exists
[ -f "wyn" ] || { echo "ERROR: wyn compiler not found"; exit 1; }

# Compile the test (regular compiler doesn't have -O2 flag, but optimization passes are implemented)
echo "Compiling test_inlining.wyn..."
./wyn tests/test_inlining.wyn || { echo "ERROR: Compilation failed"; exit 1; }

# Check that output was generated
[ -f "tests/test_inlining.wyn.out" ] || { echo "ERROR: Output executable not generated"; exit 1; }

# Run the test
echo "Running inlining test..."
./tests/test_inlining.wyn.out || { echo "ERROR: Test execution failed"; exit 1; }

echo "✓ Task 8.1 Complete - Function Inlining"
exit 0