#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 8.4 Validation: Tail Call Optimization ==="

# Check that the test file exists
[ -f "tests/test_tco.wyn" ] || { echo "ERROR: test_tco.wyn not found"; exit 1; }

# Check that compiler exists
[ -f "wyn" ] || { echo "ERROR: wyn compiler not found"; exit 1; }

# Compile the test
echo "Compiling test_tco.wyn..."
./wyn tests/test_tco.wyn || { echo "ERROR: Compilation failed"; exit 1; }

# Check that output was generated
[ -f "tests/test_tco.wyn.out" ] || { echo "ERROR: Output executable not generated"; exit 1; }

# Run the test
echo "Running TCO test..."
./tests/test_tco.wyn.out || { echo "ERROR: Test execution failed"; exit 1; }

echo "✓ Task 8.4 Complete - Tail Call Optimization"
exit 0