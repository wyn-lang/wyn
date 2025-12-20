#!/bin/sh
# Wyn Language Installer for Unix/Linux/macOS
# Usage: curl -fsSL https://wynlang.com/install.sh | sh

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "${GREEN}Wyn Language Installer${NC}"
echo ""

# Detect OS
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
case "$OS" in
  darwin) OS="macos" ;;
  linux) OS="linux" ;;
  *) echo "${RED}Error: Unsupported OS: $OS${NC}"; exit 1 ;;
esac

# Detect architecture
ARCH=$(uname -m)
case "$ARCH" in
  x86_64|amd64) ARCH="x86_64" ;;
  aarch64|arm64) ARCH="arm64" ;;
  *) echo "${RED}Error: Unsupported architecture: $ARCH${NC}"; exit 1 ;;
esac

PLATFORM="${OS}-${ARCH}"
echo "Detected platform: ${YELLOW}${PLATFORM}${NC}"

# Get latest release version
echo "Fetching latest version..."
VERSION=$(curl -fsSL https://api.github.com/repos/wyn-lang/wyn/releases/latest | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')

if [ -z "$VERSION" ]; then
  echo "${RED}Error: Could not fetch latest version${NC}"
  exit 1
fi

echo "Latest version: ${GREEN}${VERSION}${NC}"

# Download URL
URL="https://github.com/wyn-lang/wyn/releases/download/${VERSION}/wyn-${VERSION}-${PLATFORM}.tar.gz"
TEMP_DIR=$(mktemp -d)
TEMP_FILE="${TEMP_DIR}/wyn.tar.gz"

echo "Downloading from: ${URL}"
if ! curl -fsSL "$URL" -o "$TEMP_FILE"; then
  echo "${RED}Error: Download failed${NC}"
  echo "URL: $URL"
  exit 1
fi

# Extract
echo "Extracting..."
tar -xzf "$TEMP_FILE" -C "$TEMP_DIR"

# Install
INSTALL_DIR="/usr/local"
echo "Installing to ${INSTALL_DIR}..."

# Check if we need sudo
if [ -w "$INSTALL_DIR/bin" ]; then
  SUDO=""
else
  SUDO="sudo"
  echo "${YELLOW}Note: sudo required for installation${NC}"
fi

# Install binary
$SUDO mkdir -p "$INSTALL_DIR/bin"
$SUDO cp "$TEMP_DIR/release/wyn" "$INSTALL_DIR/bin/wyn"
$SUDO chmod +x "$INSTALL_DIR/bin/wyn"

# Install standard library
if [ -d "$TEMP_DIR/release/std" ]; then
  $SUDO mkdir -p "$INSTALL_DIR/share/wyn"
  $SUDO cp -r "$TEMP_DIR/release/std" "$INSTALL_DIR/share/wyn/"
fi

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo "${GREEN}✅ Wyn ${VERSION} installed successfully!${NC}"
echo ""
echo "Verify installation:"
echo "  ${YELLOW}wyn --version${NC}"
echo ""
echo "Get started:"
echo "  ${YELLOW}wyn --help${NC}"
echo ""
echo "Documentation: ${YELLOW}https://wynlang.com/docs${NC}"
