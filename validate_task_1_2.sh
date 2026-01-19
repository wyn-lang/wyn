#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/benchmarking.c" ] || exit 1
! grep -q "benchmarking.c" Makefile || exit 1
make clean && make wyn || exit 1
echo "✓ Task 1.2 Complete"