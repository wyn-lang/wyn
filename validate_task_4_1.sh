#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
./wyn tests/test_generic_structs.wyn || exit 1
./tests/test_generic_structs.wyn.out || exit 1
echo "✓ Task 4.1 Complete"