# LLVM Backend

Wyn now uses LLVM as the primary backend for multi-platform support.

## Why LLVM?

- **Multi-platform**: Single IR targets ARM64, x86_64, RISC-V, etc.
- **Optimizations**: LLVM's world-class optimizer
- **Tooling**: Integrates with existing LLVM ecosystem
- **Future-proof**: Easy to add new targets

## Usage

```bash
# Default (LLVM backend)
./build/stage0 program.wyn

# Explicit LLVM
./build/stage0 --backend llvm program.wyn

# Native ARM64 (legacy)
./build/stage0 --backend native program.wyn

# Cross-compile to x86_64
./build/stage0 --backend llvm --target x86_64 program.wyn
```

## Installation

### macOS
```bash
brew install llvm
```

### Linux
```bash
# Ubuntu/Debian
sudo apt install llvm clang

# Fedora
sudo dnf install llvm clang
```

## Makefile Integration

```bash
# Build with LLVM (default)
make all

# Build with native backend
make BACKEND=native all

# Custom LLVM path
make LLVM_PREFIX=/usr/local/opt/llvm all
```

## Implementation Status

| Feature | Status |
|---------|--------|
| Basic IR generation | ✅ Complete |
| Function calls | 🚧 In progress |
| Variables | 🚧 In progress |
| Control flow | 🚧 In progress |
| Structs | ⏳ Planned |
| Traits | ⏳ Planned |

## Architecture

```
Wyn Source → Parser → AST → LLVM IR → LLC → Object → Linker → Executable
```

**Benefits:**
- Single codegen for all platforms
- LLVM handles platform-specific details
- Easy to add optimizations

## Next Steps

1. Complete LLVM IR generation for all AST nodes
2. Add runtime library integration
3. Implement optimization passes
4. Add debug info generation
5. Support for all platforms (Linux, Windows, BSD)
