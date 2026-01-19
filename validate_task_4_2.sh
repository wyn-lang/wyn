#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
./wyn tests/test_trait_bounds.wyn || exit 1
./tests/test_trait_bounds.wyn.out || exit 1
echo "✓ Task 4.2 Complete"