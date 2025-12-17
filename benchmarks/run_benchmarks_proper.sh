#!/bin/bash
# Comprehensive Wyn benchmarks - execution time only (no compilation)

echo "=== Wyn Performance Benchmarks ==="
echo "Platform: $(uname -m) $(uname -s)"
echo "Date: $(date +%Y-%m-%d)"
echo ""

# Compile all Wyn benchmarks first
echo "Compiling Wyn benchmarks..."
../build/wyn compile -o /tmp/wyn_concurrency concurrency_bench.wyn 2>/dev/null
../build/wyn compile -o /tmp/wyn_cpu cpu_bench.wyn 2>/dev/null
../build/wyn compile -o /tmp/wyn_array array_bench.wyn 2>/dev/null
../build/wyn compile -o /tmp/wyn_fib cpu/fib.wyn 2>/dev/null
../build/wyn compile -o /tmp/wyn_primes cpu/primes.wyn 2>/dev/null
echo "✓ Compilation complete"
echo ""

# Compile Go benchmarks
echo "Compiling Go benchmarks..."
go build -o /tmp/go_concurrency concurrency/concurrency.go 2>/dev/null
go build -o /tmp/go_cpu cpu/fib.go 2>/dev/null
go build -o /tmp/go_primes cpu/primes.go 2>/dev/null
echo "✓ Go compilation complete"
echo ""

# Compile Java benchmarks
echo "Compiling Java benchmarks..."
javac -d /tmp cpu/Fib.java 2>/dev/null
echo "✓ Java compilation complete"
echo ""

echo "========================================"
echo "BENCHMARK 1: Concurrency (1000 tasks)"
echo "========================================"
echo ""

echo "Wyn:"
time /tmp/wyn_concurrency 2>&1 | grep -v Runtime

echo ""
echo "Python (threading):"
time python3 concurrency_bench.py

echo ""
echo "Go (goroutines):"
time /tmp/go_concurrency

echo ""
echo "========================================"
echo "BENCHMARK 2: CPU - Fibonacci(35)"
echo "========================================"
echo ""

echo "Wyn:"
time /tmp/wyn_fib 2>&1 | grep -v Runtime

echo ""
echo "Python:"
time python3 cpu_bench.py

echo ""
echo "Go:"
time /tmp/go_cpu

echo ""
echo "Java:"
time java -cp /tmp Fib

echo ""
echo "========================================"
echo "BENCHMARK 3: CPU - Primes (100K)"
echo "========================================"
echo ""

echo "Wyn:"
time /tmp/wyn_primes 2>&1 | grep -v Runtime

echo ""
echo "Python:"
time python3 cpu/primes.py

echo ""
echo "Go:"
time /tmp/go_primes

echo ""
echo "========================================"
echo "BENCHMARK 4: Array Operations (1M)"
echo "========================================"
echo ""

echo "Wyn:"
time /tmp/wyn_array 2>&1 | grep -v Runtime

echo ""
echo "Python:"
time python3 array_bench.py

echo ""
echo "=== Benchmark Complete ==="
