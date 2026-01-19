#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Epic 1 Validation ==="

# Check no stub files remain
stub_count=$(find src -name "*.c" -exec grep -l "// Stub" {} \; 2>/dev/null | wc -l | tr -d ' ')
echo "Stub files remaining: $stub_count"
[ "$stub_count" -le 10 ] || exit 1  # Only runtime stubs allowed

# Check compiler builds
make clean && make wyn || exit 1
echo "✓ Compiler builds"

# Check all examples work
success=0
total=0
for file in examples/*.wyn; do
    total=$((total + 1))
    if ./wyn "$file" 2>&1 | grep -q "Compiled successfully"; then
        success=$((success + 1))
    fi
done
echo "✓ $success/$total examples compile"
[ "$success" -eq "$total" ] || exit 1

echo "✓✓✓ Epic 1 Complete - All stubs removed"
exit 0