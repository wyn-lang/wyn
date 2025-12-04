# Wyn Programming Language

**One language to rule them all.**

Fast, simple, universal. **20-30x faster than Python.**

## Quick Install

```bash
git clone https://github.com/wyn-lang/wyn.git
cd wyn
./install.sh
```

Or build manually:
```bash
make all
sudo cp build/stage0 /usr/local/bin/wyn  # Installs Stage 1 compiler as 'wyn'
```

## Quick Start

```wyn
fn main() {
    io.print_str("Hello, World!")
    io.print_newline()
}
```

```bash
# Compile and run
wyn -o hello hello.wyn
./hello

# With optimizations (recommended)
wyn --stage1-tc --stage1-opt -o hello hello.wyn
```

## Performance

**Wyn competes with C and Go, beats Java and Node.js, destroys Python:**

- **vs Go**: 2.3x faster on arrays, 1.6x faster startup
- **vs Java**: 45x faster startup, 2.5x faster on arrays
- **vs Node.js**: 8.4x faster on math, 25.6x faster startup
- **vs C**: Matches on arrays (0.06s), near-instant startup
- **vs Python**: 20-30x faster with native compilation

**The best of all worlds:**
- ✅ **Fast as C/Go** - Native compilation, optimized binaries
- ✅ **Simple as Python** - Clean syntax, easy to learn
- ✅ **Versatile** - Scripts, CLI tools, servers, systems programming
- ✅ **Complete** - 500+ stdlib functions, 100% test coverage

**No reason to use anything else anymore.**

See [benchmarks/FINAL_RESULTS.md](benchmarks/FINAL_RESULTS.md) for details.

## Features

- **Fast** - Compiles to native code, 20-30x faster than Python
- **Simple** - Python-like syntax, easy to learn
- **Universal** - One language for everything: scripts, CLI, servers, systems
- **Module system** - Clean organization (io, fs, math, http, gui, mobile)
- **Type safe** - Explicit types, no surprises
- **Stage 1** - Enhanced type checking & optimizations
- **Native GUI** - Built-in GUI support using platform APIs (macOS: Cocoa)
- **iOS Mobile** - Native iOS apps with UIKit bindings
- **Run anywhere** - Compile to binary OR run as script
- **No dependencies** - Single binary, no runtime needed

**One language to rule them all.**

## Stage 1 Compiler

The `wyn` compiler includes Stage 1 features (built into the binary):

```bash
# Type inference + better errors
wyn --stage1-tc -o output input.wyn

# Optimizations (constant folding, dead code elimination, inlining)
wyn --stage1-opt -o output input.wyn

# Both (recommended for production)
wyn --stage1-tc --stage1-opt -o output input.wyn
```

Stage 1 provides 28-42% performance improvements and enhanced developer experience.

## What Works

- ✅ CLI tools
- ✅ HTTP servers
- ✅ TUI applications
- ✅ GUI applications (macOS)
- ✅ iOS mobile apps
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
- [GUI Support](docs/gui-support.md)
- [iOS Mobile](docs/mobile-ios.md)
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

# GUI window (macOS)
./build/stage0 -o gui examples/gui_window.wyn

# iOS mobile app (requires iOS SDK)
./scripts/build_ios.sh examples/mobile_ios_app.wyn MyApp
```

## License

MIT
