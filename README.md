# Wyn Programming Language

**Official Website:** [wynlang.com](https://wynlang.com)

Fast, compiled systems language with clean, expressive syntax, powered by LLVM.

## Quick Start

```bash
# Build compiler
make

# Run tests (54/54 passing)
make test

# Run a program
./build/wyn run examples/hello.wyn
```

## Example

```wyn
fn main() {
    print("Hello, Wyn!")
}
```

## New in v0.3.0

### Method Syntax
```wyn
"hello".upper()              // "HELLO"
42.to_str()                  // "42"
[1,2,3].len()                // 3
```

### Extension Methods (Python/JavaScript style)
```wyn
fn int.squared(self) -> int {
    return self * self
}

5.squared()  // 25
```

## Features

- ⚡ **Fast** - LLVM-powered native compilation
- 🎯 **Simple** - Clean, expressive syntax with method syntax
- 🌍 **Universal** - CLI, servers, GUI, mobile
- 🛡️ **Type Safe** - Explicit types with compile-time checking
- 📦 **Complete** - 120+ stdlib functions across 19 modules
- 🚀 **Production Ready** - 54/54 tests passing (100%)
- 💎 **Extensible** - Add custom methods to any type

## Standard Library

19 modules with 120+ functions:
- **fs** - File operations
- **net** - HTTP, TCP, UDP
- **os** - Environment, process
- **time** - Timestamps, sleep
- **json** - JSON parsing
- **crypto** - Encryption
- **collections** - HashMap, Set
- And more...

## Documentation

Visit [wynlang.com](https://wynlang.com) for complete documentation, examples, and guides.

## Building

```bash
make          # Build compiler
make test     # Run tests (54/54 passing)
make clean    # Clean build
```

## License

MIT - See LICENSE file
