#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/ecosystem.c" ] || exit 1
[ ! -f "src/ecosystem_thirdparty.c" ] || exit 1
[ ! -f "src/community.c" ] || exit 1
! grep -q "ecosystem.c" Makefile || exit 1
make clean && make wyn || exit 1
./wyn examples/01_hello_world.wyn && ./examples/01_hello_world.wyn.out || exit 1
echo "✓ Task 1.1 Complete"