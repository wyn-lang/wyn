# Wyn Programming Language

**One language to rule them all.**

Fast, simple, universal. **20-30x faster than Python.**

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

## Performance

**Wyn is 20-30x faster than Python:**
- Fibonacci: **23.5x faster**
- Arrays: **29.5x faster**
- Math: **19.7x faster**

**Compared to other languages:**
- Similar to Java and Node.js
- 2-3x slower than C/Go (acceptable for high-level language)

See [benchmarks/FINAL_RESULTS.md](benchmarks/FINAL_RESULTS.md) for details.

## Features

- **Fast** - Compiles to native code, 20-30x faster than Python
- **Simple** - Python-like syntax
- **Universal** - One language for everything
- **Module system** - Clean organization (io, fs, math, http)
- **Type safe** - Explicit types, no surprises
- **Stage 1** - Enhanced type checking & optimizations

## Stage 1 Compiler

Enhanced compiler with better developer experience:

```bash
# Type inference + better errors
./build/stage0 --stage1-tc -o output input.wyn

# Optimizations (constant folding, dead code elimination)
./build/stage0 --stage1-opt -o output input.wyn

# Both (recommended)
./build/stage0 --stage1-tc --stage1-opt -o output input.wyn
```

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
- **Stage 1 complete**
- **20-30x faster than Python**

## Documentation

- [Getting Started](docs/1-getting-started.md)
- [Language Spec](docs/2-language-spec.md)
- [Package Manager](docs/3-package-manager.md)
- [Stdlib Reference](docs/4-stdlib-reference.md)
- [Stage 1 Type Checker](docs/stage1-type-checker.md)
- [Stage 1 Optimizations](docs/stage1-optimizations.md)
- [Stage 1 Inlining](docs/stage1-inlining.md)
- [Performance Benchmarks](benchmarks/FINAL_RESULTS.md)

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
