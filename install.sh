#!/bin/bash
# Wyn Language Installer

set -e

echo "================================================"
echo "  Wyn Language Installer"
echo "================================================"
echo ""

# Check platform
OS=$(uname -s)
ARCH=$(uname -m)

echo "Platform: $OS $ARCH"
echo ""

# Build compiler
echo "Building Wyn compiler..."
make clean
make all

if [ ! -f "build/stage0" ]; then
    echo "Error: Compiler build failed"
    exit 1
fi

echo "✓ Compiler built successfully"
echo ""

# Run tests
echo "Running tests..."
make test

echo ""
echo "✓ All tests passed"
echo ""

# Install
echo "Installing to /usr/local/bin..."
sudo cp build/stage0 /usr/local/bin/wyn

echo ""
echo "================================================"
echo "  Installation Complete!"
echo "================================================"
echo ""
echo "Try it:"
echo "  wyn --help"
echo "  wyn -o hello examples/hello.wyn"
echo "  ./hello"
echo ""
echo "Documentation: docs/"
echo "Examples: examples/"
echo ""
