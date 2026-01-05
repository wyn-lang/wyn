#!/bin/bash
# Wyn build script

set -e

if [ "$1" = "clean" ]; then
    make clean
    echo "Cleaned build artifacts"
    exit 0
fi

if [ "$1" = "test" ]; then
    make test
    ./tests/integration_tests.sh
    exit 0
fi

if [ "$1" = "all" ]; then
    make clean
    make wyn
    make test
    ./tests/integration_tests.sh
    echo "✅ Build complete - all tests passing"
    exit 0
fi

if [ -z "$1" ]; then
    echo "Usage: ./build.sh [clean|test|all|<file.wyn>]"
    exit 1
fi

# Compile and run a Wyn file
./wyn "$1"
if [ -f "$1.out" ]; then
    ./"$1.out"
fi
