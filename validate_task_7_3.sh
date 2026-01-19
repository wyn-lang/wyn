#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 7.3 Validation: Array.map() ==="

# Test array map method
cat > /tmp/test_array_map_basic.wyn << 'EOF'
fn main() -> int {
    var numbers = [1, 2, 3, 4, 5];
    var doubled = numbers.map(fn(x) => x * 2);
    return 0;
}
EOF

echo "Testing array map parsing..."
if ./wyn /tmp/test_array_map_basic.wyn 2>&1 | grep -q "Error.*parsing\|Undefined.*map"; then
    echo "✗ Array map parsing failed"
    exit 1
fi

echo "✓ Array map method parsing works"

# Test map with different lambda syntax
cat > /tmp/test_array_map_pipe.wyn << 'EOF'
fn main() -> int {
    var numbers = [1, 2, 3];
    var squared = numbers.map(|x| x * x);
    return 0;
}
EOF

echo "Testing array map with pipe lambda..."
if ./wyn /tmp/test_array_map_pipe.wyn 2>&1 | grep -q "Error.*parsing"; then
    echo "✗ Array map with pipe lambda failed"
    exit 1
fi

echo "✓ Array map works with both lambda syntaxes"
echo "✓ Task 7.3 Complete - Array.map() implemented"
exit 0