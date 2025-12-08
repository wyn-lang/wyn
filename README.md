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

### Builtins (8 only)

1. `print()` - Output
2. `assert()` - Testing
3. `exit()` - Process control
4. `args()` - Command line arguments
5. `int_to_str()` - String conversion
6. `system()` - Shell commands
7. `write_file()` - File writing
8. `substring()` - String slicing
9. `ord()` - Character code

**Everything else requires imports.**

**Target:** Reduce to 1 builtin (print only) via syscalls.

### Standard Library

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
4. Reduce to 1 builtin (print only)

## Documentation

See `docs/` for language reference and guides.

## License

See LICENSE file.
