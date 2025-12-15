#!/bin/bash
# Cached test runner - only recompiles when source changes

CACHE_DIR=".wyn/test-cache"
mkdir -p "$CACHE_DIR"

test_file() {
    local file=$1
    local name=$(basename "$file" .wyn)
    local hash=$(md5 -q "$file" 2>/dev/null || md5sum "$file" | cut -d' ' -f1)
    local cached_hash="$CACHE_DIR/$name.hash"
    local cached_bin="$CACHE_DIR/$name"
    
    # Check if cached version is up to date
    if [ -f "$cached_hash" ] && [ -f "$cached_bin" ]; then
        local old_hash=$(cat "$cached_hash")
        if [ "$hash" = "$old_hash" ]; then
            # Use cached binary
            if timeout 5 "$cached_bin" > /dev/null 2>&1; then
                echo "✅ $file (cached)"
                return 0
            else
                echo "❌ $file (cached)"
                return 1
            fi
        fi
    fi
    
    # Compile
    if ./build/wync "$file" -o "$cached_bin" 2>&1 | grep -q "Compiled to:"; then
        echo "$hash" > "$cached_hash"
        if timeout 5 "$cached_bin" > /dev/null 2>&1; then
            echo "✅ $file"
            return 0
        else
            echo "❌ $file"
            return 1
        fi
    else
        echo "❌ $file (compile failed)"
        return 1
    fi
}

echo "Cached Test Runner"
echo "=================="
echo ""

passed=0
failed=0

echo "Testing examples..."
for f in examples/*.wyn; do
    if test_file "$f"; then
        ((passed++))
    else
        ((failed++))
    fi
done

echo ""
echo "Testing test files..."
for f in tests/*_test.wyn; do
    [ -f "$f" ] || continue
    if test_file "$f"; then
        ((passed++))
    else
        ((failed++))
    fi
done

echo ""
echo "Total: $passed passed, $failed failed"

if [ $failed -eq 0 ]; then
    echo "✅ All tests passed!"
    exit 0
else
    echo "❌ $failed tests failed"
    exit 1
fi
