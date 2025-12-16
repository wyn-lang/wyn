#!/bin/bash
# Fast parallel test runner

echo "Running tests in parallel..."

# Clean up old temp files
rm -f /tmp/wyn_pass_* /tmp/wyn_fail_* 2>/dev/null

# Run all tests in background with unique temp files
for f in tests/*_test.wyn; do
    (
        # Use unique temp file per test
        test_name=$(basename "$f" .wyn)
        temp_out="/tmp/wyn_test_${test_name}_$$_$RANDOM"
        
        if timeout 5 ./build/wyn compile "$f" -o "$temp_out" >/dev/null 2>&1; then
            if timeout 2 "$temp_out" >/dev/null 2>&1; then
                echo "  ✅ $f"
                touch "/tmp/wyn_pass_${test_name}_$$"
            else
                echo "  ❌ $f (runtime)"
                touch "/tmp/wyn_fail_${test_name}_$$"
            fi
        else
            echo "  ❌ $f (compile)"
            touch "/tmp/wyn_fail_${test_name}_$$"
        fi
        rm -f "$temp_out"
    ) &
done

# Wait for all background jobs
wait

# Count results
passed=$(ls /tmp/wyn_pass_* 2>/dev/null | wc -l | tr -d ' ')
failed=$(ls /tmp/wyn_fail_* 2>/dev/null | wc -l | tr -d ' ')
rm -f /tmp/wyn_pass_* /tmp/wyn_fail_*

echo ""
echo "Tests: $passed passed, $failed failed"

# Exit with error if any failed
[ "$failed" -eq 0 ]
