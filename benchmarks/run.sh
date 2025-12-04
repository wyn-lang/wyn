#!/bin/bash
# Comprehensive Wyn Benchmarks

set -e

echo "================================================"
echo "  Wyn Performance Benchmarks"
echo "================================================"
echo ""

# Compile
echo "Compiling..."
../build/stage0 --stage1-tc --stage1-opt -q -o cpu/fib_wyn cpu/fib.wyn
../build/stage0 --stage1-opt -q -o memory/array_wyn memory/array_ops_large.wyn
../build/stage0 --stage1-opt -q -o cpu/math_wyn cpu/math_intensive.wyn
cc -O2 -o cpu/fib_c cpu/fib.c 2>/dev/null
go build -o cpu/fib_go cpu/fib.go 2>/dev/null
cd cpu && javac Fib.java 2>/dev/null && cd .. || true

echo ""
echo "1. Fibonacci(35) - Recursive CPU"
echo "================================="
echo ""
echo "Wyn:"
/usr/bin/time -p ./cpu/fib_wyn 2>&1
echo ""
echo "C (-O2):"
/usr/bin/time -p ./cpu/fib_c 2>&1
echo ""
echo "Go:"
/usr/bin/time -p ./cpu/fib_go 2>&1
echo ""
echo "Java:"
/usr/bin/time -p java -cp cpu Fib 2>&1
echo ""
echo "Node.js:"
/usr/bin/time -p node -e "function fib(n){if(n<=1)return n;return fib(n-1)+fib(n-2)}console.log('fib(35) =',fib(35))" 2>&1
echo ""
echo "Python:"
/usr/bin/time -p python3 -c "def fib(n):
 if n<=1:return n
 return fib(n-1)+fib(n-2)
print(f'fib(35) = {fib(35)}')" 2>&1

echo ""
echo "2. Array Operations - 10M iterations"
echo "====================================="
echo ""
echo "Wyn:"
/usr/bin/time -p ./memory/array_wyn 2>&1
echo ""
echo "Node.js:"
/usr/bin/time -p node -e "const a=[1,2,3,4,5,6,7,8,9,10];let s=0;for(let i=0;i<10000000;i++)for(let j=0;j<a.length;j++)s+=a[j];console.log('Array sum:',s)" 2>&1
echo ""
echo "Python:"
/usr/bin/time -p python3 -c "a=[1,2,3,4,5,6,7,8,9,10];s=0
for _ in range(10000000):
 for j in range(len(a)):s+=a[j]
print(f'Array sum: {s}')" 2>&1

echo ""
echo "3. Math Operations - 10M iterations"
echo "===================================="
echo ""
echo "Wyn:"
/usr/bin/time -p ./cpu/math_wyn 2>&1
echo ""
echo "Node.js:"
/usr/bin/time -p node -e "let s=0;for(let i=0;i<10000000;i++){s+=Math.sin(i)+Math.cos(i)+Math.sqrt(i)}console.log('Math result:',s)" 2>&1
echo ""
echo "Python:"
/usr/bin/time -p python3 -c "import math
s=0.0
for i in range(10000000):s+=math.sin(i)+math.cos(i)+math.sqrt(i)
print(f'Math result: {s}')" 2>&1

# Cleanup
rm -f cpu/fib_wyn cpu/fib_c cpu/fib_go cpu/Fib.class memory/array_wyn cpu/math_wyn

echo ""
echo "================================================"
echo "Results saved to benchmarks/FINAL_RESULTS.md"
echo "================================================"
