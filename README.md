# Wyn Programming Language

**Official Website:** [wynlang.com](https://wynlang.com)

Fast, compiled systems language with clean, expressive syntax, powered by LLVM.

## Quick Start

```bash
# Build compiler
make

# Run tests
make test

# Try the REPL
./build/wyn repl

# Run a program
./build/wyn run examples/hello.wyn
```

## Example

```wyn
fn main() {
    print("Hello, Wyn!")
}
```

## Features

- ⚡ **Fast** - LLVM-powered native compilation
- 🎯 **Simple** - Clean, expressive syntax
- 🌍 **Universal** - CLI, servers, GUI, mobile
- 🛡️ **Type Safe** - Explicit types with compile-time checking
- 📦 **Complete** - 500+ stdlib functions
- 🚀 **Production Ready** - 47/48 tests passing

## Documentation

Visit [wynlang.com](https://wynlang.com) for complete documentation, examples, and guides.

## Building

```bash
make          # Build compiler
make test     # Run tests
make clean    # Clean build
```

## License

MIT - See LICENSE file
