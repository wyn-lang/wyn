#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

# Check for stub comments in optional.c
if grep -q "// Stub\|Stub implementation" src/optional.c 2>/dev/null; then
    stub_count=$(grep -c "// Stub\|Stub implementation" src/optional.c)
    echo "✗ Task 6.4 Failed - Found $stub_count stubs in optional.c"
    exit 1
fi

# Check that optional.c compiles
make clean && make wyn || exit 1

echo "✓ Task 6.4 Complete"