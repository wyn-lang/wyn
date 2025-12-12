#!/bin/bash
# Install Wyn LSP server for IDE support

echo "Installing Wyn LSP server..."

# Build if needed
if [ ! -f build/wyn-lsp ]; then
    echo "Building LSP server..."
    make
fi

# Install to system
sudo cp build/wyn-lsp /usr/local/bin/
sudo chmod +x /usr/local/bin/wyn-lsp

echo "✅ LSP server installed to /usr/local/bin/wyn-lsp"
echo ""
echo "Next steps:"
echo ""
echo "For VS Code:"
echo "  cp -r editors/vscode ~/.vscode/extensions/wyn-language"
echo "  cd ~/.vscode/extensions/wyn-language && npm install"
echo ""
echo "For Neovim:"
echo "  cp editors/nvim/wyn.lua ~/.config/nvim/lua/"
echo "  Add 'require(\"wyn\")' to init.lua"
echo ""
echo "For testing:"
echo "  echo '{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}' | wyn-lsp"
echo ""
echo "See docs/LSP_SETUP.md for detailed instructions"
