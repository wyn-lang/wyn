#!/bin/bash
# End-to-end integration tests

set -e

echo "=== Integration Tests ==="

mkdir -p temp

# Test 1: Simple expression
echo -n "Test 1: Simple expression... "
echo "1 + 2" > temp/test1.wyn
./wyn temp/test1.wyn > /dev/null 2>&1
result=$(./temp/test1.wyn.out; echo $?)
if [ "$result" = "3" ]; then
    echo "✓"
else
    echo "✗ (expected 3, got $result)"
    exit 1
fi

# Test 2: Function call
echo -n "Test 2: Function call... "
cat > temp/test2.wyn << 'EOF'
fn add(a: int, b: int) -> int {
    return a + b
}
fn main() -> int {
    return add(10, 20)
}
EOF
./wyn temp/test2.wyn > /dev/null 2>&1
result=$(./temp/test2.wyn.out; echo $?)
if [ "$result" = "30" ]; then
    echo "✓"
else
    echo "✗ (expected 30, got $result)"
    exit 1
fi

# Test 3: Variables
echo -n "Test 3: Variables... "
cat > temp/test3.wyn << 'EOF'
fn main() -> int {
    var x = 5
    var y = 10
    return x + y
}
EOF
./wyn temp/test3.wyn > /dev/null 2>&1
result=$(./temp/test3.wyn.out; echo $?)
if [ "$result" = "15" ]; then
    echo "✓"
else
    echo "✗ (expected 15, got $result)"
    exit 1
fi

# Test 4: Nested expressions
echo -n "Test 4: Nested expressions... "
cat > temp/test4.wyn << 'EOF'
fn main() -> int {
    return 2 * 3 + 4
}
EOF
./wyn temp/test4.wyn > /dev/null 2>&1
result=$(./temp/test4.wyn.out; echo $?)
if [ "$result" = "10" ]; then
    echo "✓"
else
    echo "✗ (expected 10, got $result)"
    exit 1
fi

# Test 4: Nested expressions
echo -n "Test 4: Nested expressions... "
cat > temp/test4.wyn << 'EOF'
fn main() -> int {
    return 2 * 3 + 4
}
EOF
./wyn temp/test4.wyn > /dev/null 2>&1
result=$(./temp/test4.wyn.out; echo $?)
if [ "$result" = "10" ]; then
    echo "✓"
else
    echo "✗ (expected 10, got $result)"
    exit 1
fi

# Test 5: Enums and match
echo -n "Test 5: Enums and match... "
cat > temp/test5.wyn << 'EOF'
enum Status { A, B, C }
fn main() -> int {
    var s = B
    return match s {
        A => 1,
        B => 2,
        C => 3
    }
}
EOF
./wyn temp/test5.wyn > /dev/null 2>&1
result=$(./temp/test5.wyn.out; echo $?)
if [ "$result" = "2" ]; then
    echo "✓"
else
    echo "✗ (expected 2, got $result)"
    exit 1
fi

# Test 6: Compound assignment
echo -n "Test 6: Compound assignment... "
cat > temp/test6.wyn << 'EOF'
fn main() -> int {
    var x = 10
    x += 5
    return x
}
EOF
./wyn temp/test6.wyn > /dev/null 2>&1
result=$(./temp/test6.wyn.out; echo $?)
if [ "$result" = "15" ]; then
    echo "✓"
else
    echo "✗ (expected 15, got $result)"
    exit 1
fi

echo
echo "All integration tests passed!"
