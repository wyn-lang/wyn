#!/bin/bash
# Fast test runner - runs tests in parallel

echo "Running tests in parallel..."

passed=0
failed=0

# Run all tests in background
for f in tests/*_test.wyn; do
    (
        if timeout 5 ./build/wyn run "$f" >/dev/null 2>&1; then
            echo "  ✅ $f"
            echo "1" > /tmp/wyn_test_$$.pass
        else
            echo "  ❌ $f"
            echo "1" > /tmp/wyn_test_$$.fail
        fi
    ) &
done

# Wait for all background jobs
wait

# Count results
passed=$(ls /tmp/wyn_test_*.pass 2>/dev/null | wc -l)
failed=$(ls /tmp/wyn_test_*.fail 2>/dev/null | wc -l)
rm -f /tmp/wyn_test_*.pass /tmp/wyn_test_*.fail

echo ""
echo "Tests: $passed passed, $failed failed"
