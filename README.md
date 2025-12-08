# Wyn Programming Language

Fast, compiled systems language with Python-like syntax.

## Features

- **Fast:** Compiles to native ARM64 code
- **Simple:** Python-like syntax
- **Library-first:** Minimal global namespace (6 builtins)
- **Concurrent:** Spawn-based concurrency
- **Practical:** Built for real work

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

### Builtins (6 only)

1. `print()` - Output
2. `assert()` - Testing
3. `exit()` - Process control
4. `args()` - Command line arguments
5. `int_to_str()` - String conversion
6. `system()` - Shell commands

**Everything else requires imports.**

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

- **Compiler:** stage0 (C bootstrap)
- **Tests:** 41/64 passing (64%)
- **Platform:** ARM64 macOS
- **Stage 1:** In development (self-hosting compiler in Wyn)

## Next Steps

1. Build Stage 1 compiler (written in Wyn)
2. Self-hosting (Stage 1 compiles itself)
3. Add syscalls (pure Wyn, no C dependencies)

## Documentation

See `docs/` for language reference and guides.

## License

See LICENSE file.
