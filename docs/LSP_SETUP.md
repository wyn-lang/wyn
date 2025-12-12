# Wyn LSP Server Setup Guide

The Wyn Language Server Protocol (LSP) server provides IDE features for Wyn code.

## Installation

```bash
cd /path/to/wyn-lang
make
sudo cp build/wyn-lsp /usr/local/bin/
```

## VS Code

### Option 1: Install Extension (Recommended)

1. Copy the extension:
```bash
cp -r editors/vscode ~/.vscode/extensions/wyn-language
cd ~/.vscode/extensions/wyn-language
npm install
```

2. Reload VS Code

3. Open any `.wyn` file - LSP will activate automatically

### Option 2: Manual Configuration

Add to `.vscode/settings.json`:
```json
{
  "wyn.lsp.path": "/usr/local/bin/wyn-lsp"
}
```

## Neovim

### With nvim-lspconfig

1. Install nvim-lspconfig:
```vim
" Using vim-plug
Plug 'neovim/nvim-lspconfig'
```

2. Copy Wyn config:
```bash
cp editors/nvim/wyn.lua ~/.config/nvim/lua/
```

3. Add to init.lua:
```lua
require('wyn')
```

4. Restart Neovim

### Manual Setup

Add to init.lua:
```lua
vim.api.nvim_create_autocmd('FileType', {
  pattern = 'wyn',
  callback = function()
    vim.lsp.start({
      name = 'wyn-lsp',
      cmd = {'wyn-lsp'},
    })
  end,
})
```

## Emacs

### With lsp-mode

Add to init.el:
```elisp
(require 'lsp-mode)

(add-to-list 'lsp-language-id-configuration '(wyn-mode . "wyn"))

(lsp-register-client
 (make-lsp-client
  :new-connection (lsp-stdio-connection "wyn-lsp")
  :major-modes '(wyn-mode)
  :server-id 'wyn-lsp))

(add-hook 'wyn-mode-hook #'lsp)
```

## Sublime Text

### With LSP package

1. Install LSP package via Package Control

2. Add to LSP settings:
```json
{
  "clients": {
    "wyn": {
      "enabled": true,
      "command": ["wyn-lsp"],
      "selector": "source.wyn"
    }
  }
}
```

## Features

### Currently Supported:
- ✅ Syntax validation
- ✅ Basic keyword completion
- ✅ Document sync

### Coming Soon:
- Go-to-definition
- Hover information
- Function signatures
- Symbol search
- Rename refactoring

## Testing

Test the LSP server manually:
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"initialize"}' | wyn-lsp
```

Should respond with:
```json
{"jsonrpc":"2.0","id":1,"result":{"capabilities":{"textDocumentSync":1}}}
```

## Troubleshooting

### LSP not starting
- Check `wyn-lsp` is in PATH: `which wyn-lsp`
- Check permissions: `chmod +x /usr/local/bin/wyn-lsp`
- Check logs in editor's LSP output

### No completions
- Verify LSP is connected (check editor status bar)
- Try restarting the LSP server
- Check editor LSP configuration

### Syntax errors not showing
- Ensure document sync is enabled
- Save the file to trigger validation
- Check LSP server logs

## Development

To enhance the LSP server:
1. Edit `src/lsp_server.c`
2. Rebuild: `make`
3. Reinstall: `sudo cp build/wyn-lsp /usr/local/bin/`
4. Restart editor

## Protocol

The Wyn LSP server implements:
- JSON-RPC 2.0
- LSP 3.x protocol
- Standard LSP methods

See: https://microsoft.github.io/language-server-protocol/

## Contributing

To add LSP features:
1. Implement in `src/lsp_server.c`
2. Test with `echo '{"method":"..."}' | wyn-lsp`
3. Update this documentation
4. Submit PR

## Notes

- The LSP server is minimal but functional
- Written in C for performance
- No external dependencies
- Works with any LSP-compatible editor
- Foundation for full IDE support
