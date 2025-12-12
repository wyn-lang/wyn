#!/bin/bash
# Parallel test runner for Wyn

WYNC="./build/wync"
JOBS=${JOBS:-8}
RESULTS_FILE="/tmp/wyn_test_results_$$"

echo "Running tests with $JOBS parallel jobs..."

> "$RESULTS_FILE"

# Test a single file
test_file() {
    local file=$1
    local safe_name=$(echo "$file" | tr '/' '_' | tr '.' '_')
    local tmp_out="/tmp/wyn_test_${safe_name}_$$"
    
    if $WYNC "$file" -o "$tmp_out" >/dev/null 2>&1 && \
       timeout 5 "$tmp_out" >/dev/null 2>&1; then
        echo "✅ $file" >> "$RESULTS_FILE"
        rm -f "$tmp_out"
    else
        echo "❌ $file" >> "$RESULTS_FILE"
        rm -f "$tmp_out"
    fi
}

export WYNC
export RESULTS_FILE

# Run tests with job control
run_parallel() {
    local files=("$@")
    local running=0
    
    for file in "${files[@]}"; do
        test_file "$file" &
        ((running++))
        
        if [ $running -ge $JOBS ]; then
            wait -n
            ((running--))
        fi
    done
    
    wait
}

# Collect test files
echo "Testing examples..."
example_files=($(find examples -name "*.wyn" -type f))
run_parallel "${example_files[@]}"

echo "Testing test files..."
test_files=($(find tests -name "*_test.wyn" -type f))
run_parallel "${test_files[@]}"

# Display and count results
cat "$RESULTS_FILE" | sort

examples_passed=$(grep "examples/" "$RESULTS_FILE" | grep -c "✅" || echo 0)
examples_failed=$(grep "examples/" "$RESULTS_FILE" | grep -c "❌" || echo 0)
tests_passed=$(grep "tests/" "$RESULTS_FILE" | grep -c "✅" || echo 0)
tests_failed=$(grep "tests/" "$RESULTS_FILE" | grep -c "❌" || echo 0)

echo ""
echo "Examples: $examples_passed passed, $examples_failed failed"
echo "Tests: $tests_passed passed, $tests_failed failed"

# Cleanup
rm -f "$RESULTS_FILE" /tmp/wyn_test_*

total_failed=$((examples_failed + tests_failed))

if [ $total_failed -eq 0 ]; then
    echo "✅ Test suite complete!"
    exit 0
else
    echo "❌ $total_failed tests failed"
    exit 1
fi
