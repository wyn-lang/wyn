#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/enterprise.c" ] || exit 1
[ ! -f "src/production.c" ] || exit 1
[ ! -f "src/release.c" ] || exit 1
if grep -q "enterprise\.c\|production\.c\|release\.c\|launch\.c" Makefile; then
    echo "ERROR: Enterprise/production files still referenced in Makefile"
    exit 1
fi
make clean && make wyn || exit 1
echo "✓ Task 1.9 Complete"