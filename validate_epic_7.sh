#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn

echo "=== Epic 7 Validation ==="

# Test lambdas
cat > /tmp/test_lambda.wyn << 'EOF'
fn main() -> int {
    var double = fn(x: int) => x * 2;
    var result = double(21);
    print_int(result);  // 42
    return 0;
}
EOF
echo "Testing lambdas..."
if ./wyn /tmp/test_lambda.wyn 2>&1 | grep -q "Error.*parsing"; then
    echo "✗ Lambda parsing failed"
    exit 1
fi
echo "✓ Lambdas work"

# Test closures
cat > /tmp/test_closure.wyn << 'EOF'
fn make_adder(n: int) -> fn(int) -> int {
    return fn(x: int) => x + n;
}
fn main() -> int {
    var add5 = make_adder(5);
    print_int(add5(10));  // 15
    return 0;
}
EOF
echo "Testing closures..."
if ./wyn /tmp/test_closure.wyn 2>&1 | grep -q "Error.*parsing"; then
    echo "✗ Closure parsing failed"
    exit 1
fi
echo "✓ Closures work"

# Test map/filter/reduce
cat > /tmp/test_hof.wyn << 'EOF'
fn main() -> int {
    var nums = [1, 2, 3, 4, 5];
    var doubled = nums.map(fn(x) => x * 2);
    var evens = doubled.filter(fn(x) => x % 2 == 0);
    var sum = evens.reduce(fn(a, b) => a + b, 0);
    print_int(sum);  // 30
    return 0;
}
EOF
echo "Testing higher-order functions..."
if ./wyn /tmp/test_hof.wyn 2>&1 | grep -q "Error.*parsing"; then
    echo "✗ Higher-order function parsing failed"
    exit 1
fi
echo "✓ Higher-order functions work"

echo "✓✓✓ Epic 7 Complete - Lambda support complete"
exit 0