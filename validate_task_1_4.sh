#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/devtools.c" ] || exit 1
[ ! -f "src/repl.c" ] || exit 1
! grep -q "devtools.c\|repl.c\|formatter.c\|doc_generator.c" Makefile || exit 1
make clean && make wyn || exit 1
echo "✓ Task 1.4 Complete"