# Wyn Programming Language v0.6.0

[![Build and Test](https://github.com/wyn-lang/wyn/actions/workflows/build.yml/badge.svg)](https://github.com/wyn-lang/wyn/actions/workflows/build.yml)
[![Release](https://github.com/wyn-lang/wyn/actions/workflows/release.yml/badge.svg)](https://github.com/wyn-lang/wyn/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-5%20platforms-blue)](https://github.com/wyn-lang/wyn)
[![VSCode](https://img.shields.io/badge/VSCode-Extension-007ACC)](https://marketplace.visualstudio.com/items?itemName=wyn-lang.wyn)

**Official Website:** [wynlang.com](https://wynlang.com) | **[VSCode Extension](https://marketplace.visualstudio.com/items?itemName=wyn-lang.wyn)**

Fast, compiled systems language with clean, expressive syntax, powered by LLVM. **6x faster** on real workloads than Python. **95% self-hosted** with working module system.

## Installation

### macOS (Homebrew)
```bash
brew install wyn-lang/tap/wyn
```

### Linux/Windows
```bash
# Download from releases
curl -L https://github.com/wyn-lang/wyn/releases/latest/download/wyn-$(uname -s)-$(uname -m).tar.gz | tar xz
sudo mv wyn /usr/local/bin/
```

### From Source
```bash
git clone https://github.com/wyn-lang/wyn.git
cd wyn
make install
```

## Quick Start

```bash
# Create hello.wyn
echo 'var message = "Hello, Wyn!"
print(message)' > hello.wyn

# Run it
wyn run hello.wyn

# Or compile and run
wyn build hello.wyn
./hello
```

## Example

```wyn
var numbers = [1, 2, 3, 4, 5]
var doubled = [x * 2 for x in numbers if x > 2]  // [6, 8, 10]
var slice = numbers[1:4]  // [2, 3, 4]

fn main() {
    print("Doubled: ${doubled}")
    print("Slice: ${slice}")
}
```

## New in v0.6.0

### Module System & Self-Hosting Foundations
```wyn
import utils                    // Import modules
import compiler.lexer          // Nested module imports
import std.fs                  // Standard library modules

fn main() {
    utils.helper()             // Call module functions
    var tokens = compiler.lexer.tokenize("code")
    std.fs.write_file("output.txt", "data")
}
```

### Self-Hosting Components (95% Complete)
- **Lexer** (compiler/lexer.wyn) - 20KB of Wyn code
- **Parser** (compiler/parser.wyn) - 9KB of Wyn code  
- **Type Checker** (compiler/typechecker.wyn) - 13KB of Wyn code
- **LLVM Codegen** (compiler/codegen.wyn) - 13KB of Wyn code

### Type Inference for Module Calls
```wyn
import math

var result = math.sqrt(16.0)   // Type inferred as float
var rounded = math.round(3.7)  // Type inferred as int
```

### Enhanced Module Organization
```wyn
// File: utils/string.wyn
fn reverse(s: string) -> string { ... }

// File: main.wyn  
import utils.string
var reversed = utils.string.reverse("hello")
```

## Features

- ⚡ **6x Faster** - Outperforms Python on real workloads
- 🏗️ **95% Self-Hosted** - Compiler written in Wyn itself
- 📦 **Module System** - Clean imports and namespace organization
- 🎯 **Modern Syntax** - `var` declarations, list comprehensions, slicing
- 🌍 **Cross-Platform** - 5 platforms: macOS, Linux, Windows, ARM64, x86_64
- 🛡️ **Type Safe** - Explicit types with compile-time checking
- 📚 **Complete** - 120+ stdlib functions across 19 modules
- 🚀 **Production Ready** - All core tests passing (100%)
- 💎 **Extensible** - Add custom methods to any type
- 🔧 **Great Tooling** - VSCode extension with syntax highlighting

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

## Why Wyn?

**Familiar yet powerful** - If you know Python or JavaScript, you'll feel at home. But unlike interpreted languages, Wyn compiles to native code for serious performance.

**Self-hosting achievement** - Wyn's compiler is 95% written in Wyn itself, demonstrating the language's maturity and expressiveness for systems programming.

**Real-world fast** - While microbenchmarks vary, Wyn consistently delivers 6x speedups on actual workloads where it matters: concurrent processing, data pipelines, and server applications.

**Modern features** - Module system, list comprehensions, slicing, extension methods, and clean `var` syntax. All the expressiveness you want with the performance you need.

## Performance

Wyn delivers **6x faster** performance than Python on real-world workloads:

| Workload | Python | Wyn | Speedup |
|----------|--------|-----|---------|
| Log processing (1GB) | 15.0s | 2.5s | **6.0x** |
| JSON processing (10K objects) | 3.2s | 0.8s | **4.0x** |
| Concurrent API calls (100) | 8.5s | 1.2s | **7.1x** |
| Data pipeline (mixed I/O) | 12.3s | 2.1s | **5.9x** |
| CPU intensive (Fibonacci) | 2.3s | 1.6s | **1.4x** |

**Why Wyn is faster:**
- ⚡ **Native compilation** - LLVM-optimized machine code
- 🔄 **True parallelism** - No GIL limitations like Python
- 🛡️ **Zero-cost abstractions** - High-level features with no runtime overhead
- 📊 **1.3M tasks/sec** concurrency throughput

## Documentation

Visit [wynlang.com](https://wynlang.com) for complete documentation, examples, and guides.

## Building

```bash
make          # Build compiler
make test     # Run tests (all core tests passing)
make clean    # Clean build
```

## License

MIT - See LICENSE file
