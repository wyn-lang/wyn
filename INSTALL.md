# Installing Wyn

## Quick Install

```bash
# Clone repository
git clone https://github.com/wyn-lang/wyn.git
cd wyn

# Build and install
make
sudo make install
```

## Installation Locations

**Default installation:**
- Binary: `/usr/local/bin/wyn`
- Stdlib: `/usr/local/share/wyn/std/`

**Custom installation:**
```bash
make install PREFIX=$HOME/.local
```

This installs to:
- Binary: `$HOME/.local/bin/wyn`
- Stdlib: `$HOME/.local/share/wyn/std/`

## Verify Installation

```bash
wyn --version
wyn --help
```

## Add to PATH

If using custom PREFIX, add to your shell config:

```bash
# ~/.bashrc or ~/.zshrc
export PATH="$HOME/.local/bin:$PATH"
```

## Uninstall

```bash
sudo make uninstall
```

Or with custom PREFIX:
```bash
make uninstall PREFIX=$HOME/.local
```

## System Requirements

- **macOS:** 11.0 or later (ARM64 or x86_64)
- **Linux:** Ubuntu 20.04+ or equivalent
- **LLVM:** 14 or later

### macOS

```bash
brew install llvm
make
sudo make install
```

### Linux

```bash
sudo apt-get install llvm-14 clang-14
make
sudo make install
```

## Usage After Installation

```bash
# Create new project
wyn new myapp
cd myapp

# Run code
wyn run main.wyn

# Compile
wyn compile main.wyn -o myapp
./myapp

# Run tests
wyn test

# Start REPL
wyn repl
```

## Updating

```bash
cd wyn
git pull
make clean
make
sudo make install
```

## Troubleshooting

### Command not found

Make sure `/usr/local/bin` is in your PATH:
```bash
echo $PATH
```

### Permission denied

Use `sudo` for system-wide installation:
```bash
sudo make install
```

Or install to user directory:
```bash
make install PREFIX=$HOME/.local
```

## Summary

- ✅ Simple installation: `make && sudo make install`
- ✅ Custom location: `make install PREFIX=...`
- ✅ Easy uninstall: `sudo make uninstall`
- ✅ Works on macOS and Linux

**Wyn is ready to use system-wide!**
