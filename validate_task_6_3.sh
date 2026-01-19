#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

# Check for stub comments in async.c
if grep -q "// Stub" src/async.c 2>/dev/null; then
    stub_count=$(grep -c "// Stub" src/async.c)
    echo "✗ Task 6.3 Failed - Found $stub_count stubs in async.c"
    exit 1
else
    echo "✓ Task 6.3 Complete - No stubs in async.c"
    exit 0
fi