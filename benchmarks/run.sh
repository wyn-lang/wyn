#!/bin/bash
# Benchmark runner - compares Wyn against Python, Node, Go

set -e

echo "=== Wyn Benchmarks ==="
echo ""

# Compile Wyn benchmarks
echo "Compiling Wyn benchmarks..."
../build/stage0 -o fib_wyn fib.wyn

# Benchmark 1: Fibonacci
echo ""
echo "Benchmark: Fibonacci (35)"
echo "-------------------------"

echo -n "Wyn:    "
time ./fib_wyn

if command -v python3 &> /dev/null; then
    echo -n "Python: "
    time python3 -c "
def fib(n):
    if n <= 1: return n
    return fib(n-1) + fib(n-2)
print(f'fib(35) = {fib(35)}')
"
fi

if command -v node &> /dev/null; then
    echo -n "Node:   "
    time node -e "
function fib(n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}
console.log('fib(35) =', fib(35));
"
fi

if command -v go &> /dev/null; then
    cat > fib.go << 'EOF'
package main
import "fmt"
func fib(n int) int {
    if n <= 1 { return n }
    return fib(n-1) + fib(n-2)
}
func main() {
    fmt.Printf("fib(35) = %d\n", fib(35))
}
EOF
    go build -o fib_go fib.go
    echo -n "Go:     "
    time ./fib_go
    rm fib.go fib_go
fi

# Cleanup
rm -f fib_wyn

echo ""
echo "=== Benchmarks Complete ==="
