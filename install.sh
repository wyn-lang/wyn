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
sudo cp build/stage0 /usr/local/bin/wyn-compiler
sudo cp build/wyn /usr/local/bin/wyn-cli
sudo ln -sf /usr/local/bin/wyn-cli /usr/local/bin/wyn
sudo chmod +x /usr/local/bin/wyn-compiler /usr/local/bin/wyn-cli

echo ""
echo "================================================"
echo "  Installation Complete!"
echo "================================================"
echo ""
echo "Try it:"
echo "  wyn version"
echo "  wyn new myapp"
echo "  cd myapp && wyn run ."
echo ""
echo "Or compile directly:"
echo "  wyn-compiler -o hello examples/hello.wyn"
echo "  ./hello"
echo ""
echo "Documentation: docs/"
echo "Examples: examples/"
echo ""
