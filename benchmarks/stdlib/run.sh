#!/bin/bash
echo "=== Wyn Standard Library Benchmarks ==="
echo

for bench in *.wyn; do
    name=$(basename $bench .wyn)
    echo "Running $name..."
    ../../build/stage0 --stage1-tc --stage1-opt -o /tmp/$name $bench
    time /tmp/$name
    echo
done
