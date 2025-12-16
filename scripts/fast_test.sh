#!/bin/bash
# Fast parallel test runner

echo "Running tests in parallel..."

# Run all tests in background with unique temp files
for f in tests/*_test.wyn; do
    (
        # Use unique temp file per test
        temp_out="/tmp/wyn_test_$(basename $f .wyn)_$$"
        if timeout 5 ./build/wyn compile "$f" -o "$temp_out" >/dev/null 2>&1 && timeout 2 "$temp_out" >/dev/null 2>&1; then
            echo "  ✅ $f"
            echo "1" > "/tmp/wyn_pass_$$_$(basename $f)"
        else
            echo "  ❌ $f"
            echo "1" > "/tmp/wyn_fail_$$_$(basename $f)"
        fi
        rm -f "$temp_out"
    ) &
done

# Wait for all background jobs
wait

# Count results
passed=$(ls /tmp/wyn_pass_$$_* 2>/dev/null | wc -l | tr -d ' ')
failed=$(ls /tmp/wyn_fail_$$_* 2>/dev/null | wc -l | tr -d ' ')
rm -f /tmp/wyn_pass_$$_* /tmp/wyn_fail_$$_*

echo ""
echo "Tests: $passed passed, $failed failed"

# Exit with error if any failed
[ "$failed" -eq 0 ]
