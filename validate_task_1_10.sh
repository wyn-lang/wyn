#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/tooling.c" ] || exit 1
[ ! -f "src/performance.c" ] || exit 1
if grep -q "tooling\.c\|performance\.c\|performance_monitor\.c\|optimization\.c" Makefile; then
    echo "ERROR: Tooling/performance files still referenced in Makefile"
    exit 1
fi
make clean && make wyn || exit 1
echo "✓ Task 1.10 Complete"