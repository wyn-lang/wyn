#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/documentation.c" ] || exit 1
! grep -q "documentation.c" Makefile || exit 1
make clean && make wyn || exit 1
echo "✓ Task 1.5 Complete"