# Wyn Programming Language

Fast, compiled systems language with Python-like syntax.

## Features

- **Fast:** Compiles to native ARM64 code
- **Simple:** Python-like syntax
- **Library-first:** Minimal global namespace (8 builtins)
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
import time

fn main() {
    print("Hello, Wyn!")
    
    const data = io.read_file("file.txt")
    time.sleep_ms(1000)
    
    print(data)
}
```

## Architecture

### Builtins

**Current:** 8 builtins (temporary, due to bootstrap limitations)
**Target:** 1 builtin (print only)

**Current builtins:**
1. `print()` - Output
2. `assert()` - Testing (will move to `import test`)
3. `exit()` - Process control (will move to `import os`)
4. `args()` - Command line arguments (will move to `import os`)
5. `int_to_str()` - String conversion (will move to `import string`)
6. `system()` - Shell commands (will move to `import os`)
7. `write_file()` - File writing (will move to `import io`)
8. `substring()` - String slicing (will become string method)
9. `ord()` - Character code (will move to `import string`)

**Once Stage 1 is self-hosting, all except print() will move to stdlib via syscalls.**

**Everything else already requires imports:**

- `io` - File I/O
- `time` - Time and sleep
- `math` - Mathematical functions
- `string` - String utilities
- `os` - Operating system
- And 20+ more modules

## Building

```bash
make clean
make all
make test
```

## Status

- **Compiler:** stage0 (C bootstrap) + **Stage 1 (Wyn)** ✅
- **Tests:** 41/64 passing (64%)
- **Platform:** ARM64 macOS
- **Stage 1:** **Working!** Compiles programs with variables, expressions, functions, control flow

## Stage 1 Compiler

**Wyn compiler written in Wyn!**

```bash
# Compile Stage 1
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# Run Stage 1
./build/stage1
# Generates: build/hello_from_stage1

# Run the output
./build/hello_from_stage1
# Output: Hello from Stage 1!
```

Stage 1 successfully compiles Wyn programs with:
- Variables and expressions
- Function definitions and calls
- Control flow (if statements)
- Multiple statements

## Next Steps

1. Expand Stage 1 (full parsing, all features)
2. Self-hosting (Stage 1 compiles itself)
3. Add syscalls (pure Wyn, no C dependencies)
4. **Reduce to 1 builtin** - Move all except print() to stdlib

## The Goal

**Only `print()` as a builtin. Everything else via imports.**

```wyn
# Future: Only print() is global

import test
import os
import io
import string

fn main() {
    print("Hello!")  # Only builtin
    
    const data = io.read_file("file.txt")  # Syscall
    test.assert(data.len() > 0)  # Pure Wyn
    os.exit(0)  # Syscall
}
```

The 8 current builtins are temporary workarounds for bootstrap limitations.
Once Stage 1 is self-hosting, we'll implement everything in pure Wyn with syscalls.

## Documentation

See `docs/` for language reference and guides.

## License

See LICENSE file.
