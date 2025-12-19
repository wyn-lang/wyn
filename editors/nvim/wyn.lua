-- Wyn language support for Neovim
-- Installation:
--   1. Copy to ~/.config/nvim/lua/wyn.lua
--   2. Add to init.lua: require('wyn')
--   3. Install Wyn: sudo make install

local lspconfig = require('lspconfig')
local configs = require('lspconfig.configs')

-- Define Wyn LSP
if not configs.wyn then
  configs.wyn = {
    default_config = {
      cmd = {'wyn', 'lsp'},
      filetypes = {'wyn'},
      root_dir = function(fname)
        return lspconfig.util.find_git_ancestor(fname) or vim.fn.getcwd()
      end,
      settings = {},
    },
  }
end

-- Setup Wyn LSP
lspconfig.wyn.setup{
  on_attach = function(client, bufnr)
    -- Enable completion
    vim.api.nvim_buf_set_option(bufnr, 'omnifunc', 'v:lua.vim.lsp.omnifunc')
    
    -- Key mappings
    local opts = { noremap=true, silent=true, buffer=bufnr }
    vim.keymap.set('n', 'gd', vim.lsp.buf.definition, opts)
    vim.keymap.set('n', 'K', vim.lsp.buf.hover, opts)
    vim.keymap.set('n', '<leader>rn', vim.lsp.buf.rename, opts)
    vim.keymap.set('n', '<leader>ca', vim.lsp.buf.code_action, opts)
  end,
}

-- Filetype detection
vim.filetype.add({
  extension = {
    wyn = 'wyn',
  },
})

-- Syntax highlighting (basic)
vim.api.nvim_create_autocmd('FileType', {
  pattern = 'wyn',
  callback = function()
    vim.bo.commentstring = '# %s'
    vim.bo.shiftwidth = 4
    vim.bo.tabstop = 4
    vim.bo.expandtab = true
  end,
})

print("Wyn language support loaded!")
