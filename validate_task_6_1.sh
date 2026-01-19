#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
./wyn tests/test_future_combinators.wyn || exit 1
./tests/test_future_combinators.wyn.out || exit 1
echo "✓ Task 6.1 Complete"