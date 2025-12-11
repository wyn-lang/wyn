#!/bin/bash

echo "=== Categorizing Test Failures ==="
echo

# Categories
echo "## Missing Features:"
for f in examples/lambda_test.wyn examples/generic_test.wyn examples/default_params_test.wyn; do
    if [ -f "$f" ]; then
        echo "- $f (lambdas/generics/default params not implemented)"
    fi
done

echo
echo "## Result/Optional Type Issues:"
for f in examples/expr_test.wyn examples/env_test.wyn; do
    if [ -f "$f" ]; then
        ./build/wync "$f" 2>&1 | grep -q "Result\|Optional" && echo "- $f"
    fi
done

echo
echo "## Missing Builtins:"
for f in examples/clock_test.wyn examples/random_test.wyn; do
    if [ -f "$f" ]; then
        ./build/wync "$f" 2>&1 | grep -q "undefined variable" && echo "- $f"
    fi
done

echo
echo "## LLVM IR Issues:"
for f in examples/method_syntax_test.wyn examples/nested_struct_test.wyn; do
    if [ -f "$f" ]; then
        ./build/wync "$f" 2>&1 | grep -q "LLVM compilation failed" && echo "- $f"
    fi
done
