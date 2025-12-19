# Wyn LSP Setup

## Prerequisites

Install Wyn system-wide:
```bash
cd wyn
make
sudo make install
```

Verify:
```bash
wyn --version
wyn lsp  # Should start LSP server (Ctrl+C to exit)
```

## Neovim Setup

### 1. Install nvim-lspconfig

Using lazy.nvim:
```lua
{
  'neovim/nvim-lspconfig',
}
```

### 2. Copy Wyn config

```bash
cp editors/nvim/wyn.lua ~/.config/nvim/lua/wyn.lua
```

### 3. Load in init.lua

```lua
require('wyn')
```

### 4. Restart Neovim

Open a `.wyn` file and the LSP will start automatically.

### Key Bindings

- `gd` - Go to definition
- `K` - Hover documentation
- `<leader>rn` - Rename
- `<leader>ca` - Code actions

## VSCode Setup

### 1. Install Extension

```bash
cd editors/vscode
npm install
```

### 2. Link Extension

```bash
ln -s $(pwd) ~/.vscode/extensions/wyn-language
```

### 3. Reload VSCode

Press `Cmd+Shift+P` → "Developer: Reload Window"

### 4. Open Wyn File

The LSP will start automatically when you open a `.wyn` file.

## Testing LSP

Create a test file:
```wyn
// test.wyn
fn main() {
    const x = 42
    print(x)
}
```

You should see:
- ✅ Syntax highlighting
- ✅ Error checking
- ✅ Basic completion

## Current LSP Features

The LSP currently provides:
- ✅ Basic initialization
- ✅ File synchronization
- ⚠️ Limited diagnostics
- ⚠️ Basic completion

## Troubleshooting

### LSP not starting

Check if `wyn` is in PATH:
```bash
which wyn
wyn lsp  # Should start server
```

### Neovim: Check LSP status

```vim
:LspInfo
```

### VSCode: Check output

View → Output → Select "Wyn Language Server"

## Summary

- ✅ Neovim support with nvim-lspconfig
- ✅ VSCode support with extension
- ✅ Auto-starts when opening .wyn files
- ✅ Basic LSP features working

**Wyn LSP is ready for editor integration!**
