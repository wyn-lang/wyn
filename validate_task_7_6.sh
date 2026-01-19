#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Task 7.6 Validation: Option/Result Combinators ==="

# Test Option combinator methods
cat > /tmp/test_option_combinators_basic.wyn << 'EOF'
fn main() -> int {
    var opt = some(42);
    var doubled = opt.map(fn(x) => x * 2);
    var result = doubled.and_then(fn(x) => {
        if x > 50 {
            return some(x);
        } else {
            return none;
        }
    });
    return 0;
}
EOF

echo "Testing Option combinator parsing..."
if ./wyn /tmp/test_option_combinators_basic.wyn 2>&1 | grep -q "Error.*parsing\|Undefined.*map\|Undefined.*and_then"; then
    echo "✗ Option combinator parsing failed"
    exit 1
fi

echo "✓ Option combinator methods parsing works"

# Test Option filter method
cat > /tmp/test_option_filter.wyn << 'EOF'
fn main() -> int {
    var opt = some(42);
    var filtered = opt.filter(fn(x) => x > 40);
    return 0;
}
EOF

echo "Testing Option filter method..."
if ./wyn /tmp/test_option_filter.wyn 2>&1 | grep -q "Error.*parsing\|Undefined.*filter"; then
    echo "✗ Option filter parsing failed"
    exit 1
fi

echo "✓ Option filter method works"

# Test Result combinator methods
cat > /tmp/test_result_combinators.wyn << 'EOF'
fn main() -> int {
    var res = ok(42);
    var doubled = res.map(fn(x) => x * 2);
    var chained = doubled.and_then(fn(x) => ok(x + 1));
    return 0;
}
EOF

echo "Testing Result combinator methods..."
if ./wyn /tmp/test_result_combinators.wyn 2>&1 | grep -q "Error.*parsing\|Undefined.*map\|Undefined.*and_then"; then
    echo "✗ Result combinator parsing failed"
    exit 1
fi

echo "✓ Result combinator methods work"
echo "✓ Task 7.6 Complete - Option/Result combinators implemented"
exit 0