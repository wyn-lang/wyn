#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
./wyn tests/test_type_inference.wyn || exit 1
./tests/test_type_inference.wyn.out || exit 1
echo "✓ Task 4.4 Complete"