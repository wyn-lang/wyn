-- Wyn language support for Neovim
-- Place in ~/.config/nvim/lua/wyn.lua

local lspconfig = require('lspconfig')
local configs = require('lspconfig.configs')

-- Define Wyn LSP
if not configs.wyn_lsp then
  configs.wyn_lsp = {
    default_config = {
      cmd = {'wyn-lsp'},
      filetypes = {'wyn'},
      root_dir = function(fname)
        return lspconfig.util.find_git_ancestor(fname) or vim.fn.getcwd()
      end,
      settings = {},
    },
  }
end

-- Setup Wyn LSP
lspconfig.wyn_lsp.setup{}

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
