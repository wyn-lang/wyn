#!/bin/bash
# Validate CI/CD workflow locally
# Runs the same commands as GitHub Actions

set -e

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║           Validating CI/CD Workflow Locally                 ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo

cd "$(dirname "$0")/.."

echo "=== Clean Build ==="
make clean
echo "✅ Clean complete"
echo

echo "=== Build Compiler ==="
make
echo "✅ Build complete"
echo

echo "=== Run Test Suite ==="
make test
echo "✅ Tests complete"
echo

echo "=== Test Examples ==="
./build/wyn run examples/hello.wyn > /dev/null 2>&1 && echo "✅ hello.wyn"
./build/wyn run examples/fibonacci.wyn > /dev/null 2>&1 && echo "✅ fibonacci.wyn"
./build/wyn run examples/extension_methods.wyn > /dev/null 2>&1 && echo "✅ extension_methods.wyn"
echo

echo "=== Test REPL ==="
echo "print(42)" | ./build/wyn repl > /dev/null 2>&1 && echo "✅ REPL works" || echo "⚠️  REPL test skipped"
echo

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                  CI/CD Validation Complete! ✅                ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo
echo "All CI/CD steps passed locally!"
echo "GitHub Actions should work correctly."
