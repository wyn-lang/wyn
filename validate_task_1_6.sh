#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/build.c" ] || exit 1
if grep -q "build\.c" Makefile; then
    echo "ERROR: build.c still referenced in Makefile"
    exit 1
fi
make clean && make wyn || exit 1
echo "✓ Task 1.6 Complete"