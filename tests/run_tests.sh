#!/bin/bash
# Wyn Test Runner - Cached parallel execution

STAGE0="./build/stage0"
JOBS=${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}
CACHE_DIR=".wyn/test-cache"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

[ -f "$STAGE0" ] || make stage0
mkdir -p "$CACHE_DIR"

echo "=== Wyn Test Suite (jobs=$JOBS) ==="

# Collect tests
tests=()
for f in tests/*.wyn examples/*_test.wyn; do
    [ -f "$f" ] && tests+=("$f")
done

# Run test with caching
run_test() {
    local f="$1"
    local name=$(basename "$f" .wyn)
    local cache=".wyn/test-cache/$name"
    local hash=$(md5 -q "$f" 2>/dev/null || md5sum "$f" | cut -d' ' -f1)
    
    if grep -q "read_line\|read_input" "$f" 2>/dev/null; then
        echo "S $name"; return
    fi
    
    # Check cache
    if [ -x "$cache" ] && [ -f "$cache.hash" ] && [ "$(cat "$cache.hash")" = "$hash" ]; then
        if "$cache" >/dev/null 2>&1; then
            echo "P $name"; return
        else
            echo "F $name"; return
        fi
    fi
    
    # Compile and cache
    if ./build/stage0 -q -o "$cache" "$f" 2>/dev/null; then
        echo "$hash" > "$cache.hash"
        if "$cache" >/dev/null 2>&1; then
            echo "P $name"
        else
            echo "F $name"
        fi
    else
        echo "F $name"
    fi
}

export -f run_test

# Run in parallel
results=$(printf '%s\n' "${tests[@]}" | xargs -P "$JOBS" -I{} bash -c 'run_test "$1"' _ {})

PASS=0 FAIL=0 SKIP=0
while read -r status name; do
    case "$status" in
        P) echo -e "${GREEN}PASS${NC} $name"; ((PASS++)) ;;
        F) echo -e "${RED}FAIL${NC} $name"; ((FAIL++)) ;;
        S) echo -e "${YELLOW}SKIP${NC} $name"; ((SKIP++)) ;;
    esac
done <<< "$results"

echo -e "\n=== Results ==="
echo -e "${GREEN}Passed: $PASS${NC}"
[ $SKIP -gt 0 ] && echo -e "${YELLOW}Skipped: $SKIP${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
[ $FAIL -eq 0 ]
