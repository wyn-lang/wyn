# Wyn Neovim Plugin

Neovim plugin for the Wyn programming language with full LSP support.

## Features

- Syntax highlighting
- LSP integration (completion, go-to-definition, hover, etc.)
- Automatic filetype detection for `.wyn` files

## Installation

### Using [lazy.nvim](https://github.com/folke/lazy.nvim)

```lua
{
  'wyn-lang/wyn.nvim',
  ft = 'wyn',
  config = function()
    require('wyn').setup()
  end
}
```

### Using [packer.nvim](https://github.com/wbthomason/packer.nvim)

```lua
use {
  'wyn-lang/wyn.nvim',
  ft = 'wyn',
  config = function()
    require('wyn').setup()
  end
}
```

### Manual Installation

```bash
git clone https://github.com/wyn-lang/wyn.nvim ~/.config/nvim/pack/plugins/start/wyn.nvim
```

## Configuration

Basic setup:

```lua
require('wyn').setup()
```

Custom configuration:

```lua
require('wyn').setup({
  cmd = '/path/to/wyn',  -- Custom path to wyn executable
  on_attach = function(client, bufnr)
    -- Your custom LSP keybindings
    local opts = { noremap=true, silent=true, buffer=bufnr }
    vim.keymap.set('n', 'gd', vim.lsp.buf.definition, opts)
    vim.keymap.set('n', 'K', vim.lsp.buf.hover, opts)
    vim.keymap.set('n', 'gr', vim.lsp.buf.references, opts)
    vim.keymap.set('n', '<leader>rn', vim.lsp.buf.rename, opts)
    vim.keymap.set('n', '<leader>f', vim.lsp.buf.format, opts)
  end,
  capabilities = require('cmp_nvim_lsp').default_capabilities()
})
```

## Requirements

- Neovim 0.8.0 or higher
- [nvim-lspconfig](https://github.com/neovim/nvim-lspconfig)
- Wyn compiler installed and in PATH

## Usage

1. Install the plugin
2. Open any `.wyn` file
3. LSP features activate automatically

## LSP Features

- **Completion**: Trigger with `<C-x><C-o>` or use completion plugin
- **Go to Definition**: `gd` (with default keybindings)
- **Hover**: `K` (with default keybindings)
- **Find References**: `gr` (with default keybindings)
- **Rename**: `<leader>rn` (with default keybindings)
- **Format**: `<leader>f` (with default keybindings)

## Troubleshooting

### LSP not starting

Check if Wyn is in PATH:
```bash
which wyn
wyn lsp
```

### Syntax highlighting not working

Reload the file:
```vim
:e
```

## Development

```bash
# Clone repository
git clone https://github.com/wyn-lang/wyn.nvim

# Link to Neovim config
ln -s $(pwd) ~/.config/nvim/pack/plugins/start/wyn.nvim
```

## License

MIT
