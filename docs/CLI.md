# Wyn CLI Reference

## Overview

The `wyn` command provides a comprehensive CLI for Wyn development, inspired by Go's simplicity and Rust's cargo.

## Installation

```bash
./install.sh
```

This installs `wyn` to `/usr/local/bin`.

## Commands

### Project Management

#### `wyn init`
#### `wyn init`
Initialize a new project **in the current directory** (like `git init`).

```bash
mkdir myapp && cd myapp
wyn init
```

Creates:
- `wyn.toml` - Project configuration
- `src/main.wyn` - Entry point
- `build/` - Build directory
- `.wyn/` - Cache directory

#### `wyn new <name>`
Create a **new directory** with a project (like `cargo new`).

```bash
wyn new myapp
cd myapp
```

Creates `myapp/` directory with the same structure as `wyn init`.

**Difference**: `init` uses current directory, `new` creates a new directory.

#### `wyn run [file|.]`
Run a Wyn program.

```bash
wyn run .              # Run current project
wyn run hello.wyn      # Run specific file
```

#### `wyn build [file|.]`
Build a Wyn program.

```bash
wyn build .            # Build current project
wyn build hello.wyn    # Build specific file
```

Output goes to `build/<project-name>` or `<filename>`.

### Development

#### `wyn test`
Run tests.

```bash
wyn test               # Run project tests or Wyn test suite
```

#### `wyn clean`
Clean build artifacts.

```bash
wyn clean
```

Removes:
- Build artifacts
- Test cache
- Temporary files

#### `wyn inline "code"`
Execute inline Wyn code.

```bash
wyn inline "print(42)"
wyn inline "let x = 10; print(x * 2)"
```

Perfect for quick calculations and testing.

#### `wyn exec`
Execute Wyn code from stdin.

```bash
echo "print(42)" | wyn exec
cat script.wyn | wyn exec
```

### Utilities

#### `wyn bench`
Run performance benchmarks.

```bash
wyn bench
```

Compares Wyn against C, Go, Java, Node.js, and Python.

#### `wyn doc [topic]`
Show documentation (like manpages).

```bash
wyn doc              # Main documentation
wyn doc start        # Quick start guide
wyn doc lang         # Language specification
wyn doc stdlib       # Standard library reference
wyn doc gui          # GUI documentation
wyn doc cli          # CLI reference

# Module documentation
wyn doc fs           # Show fs module docs and function list
wyn doc io           # Show io module docs
wyn doc math         # Show math module docs

# Function documentation
wyn doc fs.read      # Show fs.read function
wyn doc io.print     # Show io.print function
wyn doc math.sin     # Show math.sin function
```

**Features**:
- Uses `bat` or `less` for paging (like `man`)
- Color-coded output
- Quick module and function lookup
- No need to open files manually

#### `wyn install`
Install wyn to system.

```bash
wyn install
```

Copies compiler to `/usr/local/bin/wyn`.

#### `wyn uninstall`
Remove wyn from system.

```bash
wyn uninstall
```

#### `wyn version`
Show version information.

```bash
wyn version
```

## Examples

### Quick Script
```bash
wyn inline "print(2 + 2)"
```

### Project Workflow
```bash
wyn new myapp
cd myapp
# Edit src/main.wyn
wyn run .
wyn build .
./build/myapp
```

### One-liner HTTP Server
```bash
wyn inline "let s = tcp_listen(8080); while true { let c = tcp_accept(s); tcp_send(c, 'HTTP/1.1 200 OK\r\n\r\nHello'); tcp_close(c) }"
```

### Pipeline Processing
```bash
cat data.txt | wyn exec
```

## Configuration

### wyn.toml

```toml
[package]
name = "myproject"
version = "0.1.0"

[build]
entry = "src/main.wyn"
```

## Compiler Options

When using `wyn` directly as compiler:

```bash
wyn -o output input.wyn                    # Basic
wyn --stage1-tc -o output input.wyn        # Type checker
wyn --stage1-opt -o output input.wyn       # Optimizations
wyn --stage1-tc --stage1-opt -o out in.wyn # Both (recommended)
```

## Tips

### Aliases
Add to your `.bashrc` or `.zshrc`:

```bash
alias wr='wyn run .'
alias wb='wyn build .'
alias wc='wyn clean'
alias wi='wyn inline'
```

### Quick Scripts
```bash
# Calculator
wyn inline "print(123 * 456)"

# File processing
wyn inline "let content = read_file('data.txt'); print(len(content))"

# Network check
wyn inline "let fd = tcp_connect('google.com', 80); print(fd)"
```

## Advanced Usage

### Custom Build Scripts
Create `build.sh`:
```bash
#!/bin/bash
wyn build .
cp build/myapp /usr/local/bin/
```

### CI/CD Integration
```yaml
# .github/workflows/build.yml
- run: ./install.sh
- run: wyn test
- run: wyn build .
```

## Troubleshooting

### "Not in a Wyn project"
Run `wyn init` to create a project.

### "Compiler not found"
Run `./install.sh` or `make all`.

### "Permission denied"
Use `sudo` for install/uninstall commands.

## See Also

- [Quick Start](QUICK_START.md)
- [Language Spec](2-language-spec.md)
- [Examples](../examples/)
