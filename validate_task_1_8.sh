#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/package.c" ] || exit 1
[ ! -f "src/package_manager.c" ] || exit 1
if grep -q "package\.c\|package_manager\.c\|package_mgr\.c" Makefile; then
    echo "ERROR: Package files still referenced in Makefile"
    exit 1
fi
make clean && make wyn || exit 1
echo "✓ Task 1.8 Complete"