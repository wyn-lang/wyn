# Wyn VS Code Extension

Official VS Code extension for the Wyn programming language.

## Features

- **Syntax Highlighting** - Full syntax highlighting for Wyn code
- **IntelliSense** - Code completion and suggestions
- **Go to Definition** - Jump to symbol definitions
- **Find References** - Find all usages of a symbol
- **Hover Information** - View type information on hover
- **Rename Symbol** - Safely rename symbols across files
- **Format Document** - Auto-format Wyn code
- **Error Diagnostics** - Real-time error checking

## Requirements

- Wyn compiler installed and in PATH
- VS Code 1.60.0 or higher

## Installation

### From VSIX (Recommended)

1. Download the latest `.vsix` file from releases
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X)
4. Click "..." menu → "Install from VSIX..."
5. Select the downloaded file

### From Source

```bash
cd vscode-extension
npm install
npm run compile
code --install-extension .
```

## Configuration

Set the path to your Wyn compiler in VS Code settings:

```json
{
  "wyn.path": "/usr/local/bin/wyn"
}
```

## Usage

1. Open any `.wyn` file
2. Extension activates automatically
3. Enjoy full language support!

## Features in Detail

### Syntax Highlighting
All Wyn keywords, types, and constructs are highlighted:
- Keywords: `fn`, `struct`, `enum`, `match`, `if`, `while`, etc.
- Types: `int`, `float`, `bool`, `string`, etc.
- Comments: `//` and `/* */`
- Strings and numbers

### IntelliSense
Type-aware code completion:
- Function signatures
- Struct fields
- Enum variants
- Keywords and types

### Go to Definition
Ctrl+Click (Cmd+Click on Mac) on any symbol to jump to its definition.

### Find References
Right-click → "Find All References" to see all usages of a symbol.

### Hover Information
Hover over any symbol to see its type and documentation.

### Rename Symbol
F2 on any symbol to rename it safely across all files.

### Format Document
Shift+Alt+F (Shift+Option+F on Mac) to format the current file.

## Troubleshooting

### Extension not activating
- Ensure Wyn compiler is installed: `wyn --version`
- Check VS Code output panel for errors

### LSP not working
- Verify LSP server starts: `wyn lsp`
- Check extension logs in Output panel

### Syntax highlighting not working
- Reload VS Code window (Ctrl+Shift+P → "Reload Window")
- Verify file extension is `.wyn`

## Development

```bash
# Install dependencies
npm install

# Compile TypeScript
npm run compile

# Watch mode
npm run watch

# Package extension
npm install -g vsce
vsce package
```

## Contributing

Contributions welcome! Please open issues or pull requests on GitHub.

## License

MIT License - see LICENSE file for details

## Links

- [Wyn Language](https://wynlang.com)
- [Documentation](https://docs.wynlang.com)
- [GitHub](https://github.com/wyn-lang/wyn)
