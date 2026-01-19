#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
echo "test content" > /tmp/test_async.txt
./wyn tests/test_async_io.wyn || exit 1
./tests/test_async_io.wyn.out || exit 1
echo "✓ Task 6.2 Complete"