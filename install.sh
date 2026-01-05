#!/bin/bash
# Wyn compiler installation script

set -e

echo "Installing Wyn compiler..."

# Build compiler
make clean
make wyn

# Install to /usr/local/bin
if [ -w /usr/local/bin ]; then
    cp wyn /usr/local/bin/wyn
    echo "✅ Installed to /usr/local/bin/wyn"
else
    echo "⚠️  Need sudo to install to /usr/local/bin"
    sudo cp wyn /usr/local/bin/wyn
    echo "✅ Installed to /usr/local/bin/wyn"
fi

# Verify installation
if command -v wyn &> /dev/null; then
    echo "✅ Wyn compiler installed successfully"
    echo "   Version: $(cat VERSION)"
    echo "   Usage: wyn <file.wyn>"
else
    echo "❌ Installation failed"
    exit 1
fi
