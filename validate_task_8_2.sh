#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 8.2 Validation: Loop Optimization ==="

# Check that the test file exists
[ -f "tests/test_loop_opt.wyn" ] || { echo "ERROR: test_loop_opt.wyn not found"; exit 1; }

# Check that compiler exists
[ -f "wyn" ] || { echo "ERROR: wyn compiler not found"; exit 1; }

# Compile the test
echo "Compiling test_loop_opt.wyn..."
./wyn tests/test_loop_opt.wyn || { echo "ERROR: Compilation failed"; exit 1; }

# Check that output was generated
[ -f "tests/test_loop_opt.wyn.out" ] || { echo "ERROR: Output executable not generated"; exit 1; }

# Run the test
echo "Running loop optimization test..."
./tests/test_loop_opt.wyn.out || { echo "ERROR: Test execution failed"; exit 1; }

echo "✓ Task 8.2 Complete - Loop Optimization"
exit 0