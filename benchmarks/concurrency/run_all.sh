#!/bin/bash
echo "=== Concurrency Benchmarks ==="
echo

echo "Wyn (spawn):"
time ../../build/stage0 --stage1-opt -o concurrent_wyn concurrent.wyn && ./concurrent_wyn
echo

echo "Go (goroutines):"
time go run concurrent.go
echo

echo "Java (threads):"
javac concurrent_java.java
time java concurrent_java
echo

echo "Node.js (async/await):"
time node concurrent_node.js
echo

echo "Python (threading):"
time python3 concurrent_python.py
