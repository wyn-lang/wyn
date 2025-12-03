# Wyn Programming Language

**One language to rule them all.**

Fast, simple, universal. Build servers, CLIs, TUIs, and more.

## Quick Start

```wyn
fn main() {
    io.print_str("Hello, World!")
    io.print_newline()
}
```

```bash
# Build
make stage0
./build/stage0 -o hello examples/hello.wyn
./hello

# Or use package manager
./wyn new myapp
cd myapp
./wyn run
```

## Features

- **Fast** - Compiles to native code
- **Simple** - Python-like syntax
- **Universal** - One language for everything
- **Module system** - Clean organization (io, fs, math, http)
- **Type safe** - Explicit types, no surprises

## What Works

- ✅ CLI tools
- ✅ HTTP servers
- ✅ TUI applications
- ✅ File processing
- ✅ Math/science programs

## Status

- **47/48 tests passing**
- **500+ stdlib functions**
- **Production ready**

## Documentation

- [Getting Started](docs/1-getting-started.md)
- [Language Spec](docs/2-language-spec.md)
- [Package Manager](docs/3-package-manager.md)
- [Stdlib Reference](docs/4-stdlib-reference.md)

## Examples

```bash
# Calculator
./build/stage0 -o calc examples/calculator.wyn

# HTTP server
./build/stage0 -o server examples/http_server.wyn

# TUI demo
./build/stage0 -o tui temp/tui_demo.wyn
```

## License

MIT
