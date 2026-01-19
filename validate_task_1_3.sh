#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/distribution.c" ] || exit 1
! grep -q "distribution.c" Makefile || exit 1
make clean && make wyn || exit 1
echo "✓ Task 1.3 Complete"