#!/bin/bash
# Wyn Test Runner

set -e

STAGE0="./build/stage0"
TESTS_DIR="tests"
EXAMPLES_DIR="examples"
PASS=0
FAIL=0
SKIP=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

run_test() {
    local file="$1"
    local name=$(basename "$file" .wyn)
    local out="/tmp/wyn_test_$$_$name"
    
    # Compile
    if ! $STAGE0 -o "$out" "$file" 2>/dev/null; then
        echo -e "${RED}FAIL${NC} $name (compile error)"
        ((FAIL++))
        return
    fi
    
    # Run and check exit code
    if "$out" >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC} $name"
        ((PASS++))
    else
        echo -e "${RED}FAIL${NC} $name (runtime error)"
        ((FAIL++))
    fi
    
    rm -f "$out"
}

echo "=== Wyn Test Suite ==="
echo ""

# Build compiler first
if [ ! -f "$STAGE0" ]; then
    echo "Building stage0..."
    make stage0
fi

# Run all test files
echo "Running tests..."
echo ""

for f in "$TESTS_DIR"/*.wyn "$EXAMPLES_DIR"/*_test.wyn; do
    [ -f "$f" ] && run_test "$f"
done

echo ""
echo "=== Results ==="
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
echo ""

[ $FAIL -eq 0 ] && exit 0 || exit 1
