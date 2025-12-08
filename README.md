# Wyn Programming Language

Fast, compiled systems language with Python-like syntax.

## Features

- **Fast:** Compiles to native ARM64 code
- **Simple:** Python-like syntax
- **Minimal:** Only 4 builtins (target: 1)
- **Concurrent:** Spawn-based concurrency
- **Self-hosting:** Stage 1 compiler written in Wyn

## Quick Start

```bash
# Compile a program
./build/stage0 -o hello examples/hello.wyn
./hello

# Or use the CLI
./build/wyn version
```

## Example

```wyn
import io
import string

fn main() {
    print("Hello, Wyn!")
    
    const data = io.read_file("file.txt")
    const code = string.ord("A")  # 65
    
    print(data)
}
```

## Builtins

**Current:** 4 builtins (87% reduction from 30+)
**Target:** 1 builtin (print only)

| Builtin | Status | Notes |
|---------|--------|-------|
| `print()` | Keep | Target builtin |
| `assert()` | Remove | Move to module |
| `args()` | Intrinsic | Compiler generates inline |
| `syscall()` | Intrinsic | Foundation for stdlib |

**Recently removed:**
- `ord()` → use `string.ord()`
- `substring()` → use slice syntax `[start:end]`

**Everything else requires imports:**
- `io` - File I/O
- `os` - Operating system
- `string` - String utilities
- `time` - Time and sleep
- `math` - Mathematical functions
- And 20+ more modules

## Building

```bash
make clean
make all
make test
```

## Status

| Component | Status |
|-----------|--------|
| Stage 0 (C bootstrap) | ✅ Complete |
| Stage 1 (Wyn compiler) | 🚧 ~15% |
| Tests | 65% passing |
| Platform | ARM64 macOS |

## Stage 1 Compiler

Wyn compiler written in Wyn:

```bash
# Compile Stage 1
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# Run Stage 1
./build/stage1

# Run output
./build/stage1_output
```

**Working features:**
- Variables (let, const)
- Expressions (arithmetic)
- Control flow (if, while, for)
- Imports (io, os, string)
- Multiple variables with tracking

**Needed for self-hosting:**
- Function parameters/returns
- Variable references in expressions
- String variables
- Full lexer/parser

## Goals

1. **1 Builtin** - Only `print()` is global
2. **Self-hosting** - Stage 1 compiles itself
3. **Pure Wyn stdlib** - All via syscalls

```wyn
# Target: Only print() is builtin
import os
import io

fn main() {
    print("Hello!")           # Only builtin
    const data = io.read_file("f.txt")  # Via syscall
    os.exit(0)                # Via syscall
}
```

## Documentation

See [`docs/`](docs/) for:
- [Quick Start](docs/quickstart.md)
- [Language Reference](docs/language.md)
- [Standard Library](docs/stdlib.md)
- [Roadmap](docs/ROADMAP.md)

## License

See LICENSE file.
