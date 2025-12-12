# Wyn Support for Neovim

## Installation

1. Install the Wyn LSP server:
```bash
cd /path/to/wyn-lang
make
sudo cp build/wyn-lsp /usr/local/bin/
```

2. Add to your Neovim config (~/.config/nvim/init.lua):
```lua
require('wyn')
```

3. Copy the Wyn config:
```bash
cp editors/nvim/wyn.lua ~/.config/nvim/lua/
```

## Features

- Syntax highlighting
- LSP integration
- Auto-completion
- Error diagnostics
- Go-to-definition (coming soon)

## Requirements

- Neovim 0.8+
- nvim-lspconfig plugin
