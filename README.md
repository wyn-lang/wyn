# Wyn Programming Language

[![Build and Test](https://github.com/wyn-lang/wyn/actions/workflows/build.yml/badge.svg)](https://github.com/wyn-lang/wyn/actions/workflows/build.yml)
[![Release](https://github.com/wyn-lang/wyn/actions/workflows/release.yml/badge.svg)](https://github.com/wyn-lang/wyn/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-5%20platforms-blue)](https://github.com/wyn-lang/wyn)

**Official Website:** [wynlang.com](https://wynlang.com)

Fast, compiled systems language with clean syntax. **6x faster** than Python on real workloads.

## Installation

### From Source
```bash
git clone https://github.com/wyn-lang/wyn.git
cd wyn
make
sudo make install
```

**Requirements:** macOS 11.0+ or Linux, LLVM 14+, Clang

## Quick Start

```bash
# Create hello.wyn
echo 'print("Hello, Wyn!")' > hello.wyn

# Run it
wyn run hello.wyn
```

## Example

```wyn
var numbers = [1, 2, 3, 4, 5]
var doubled = [x * 2 for x in numbers if x > 2]  // [6, 8, 10]

fn main() {
    print("Doubled: ${doubled}")
}
```

## Features

- ⚡ **Fast** - Native compilation via LLVM, 6x faster than Python
- 🎯 **Modern Syntax** - `var` declarations, list comprehensions, slicing
- 📦 **Module System** - Clean imports and organization
- 🌍 **Cross-Platform** - macOS, Linux, Windows, ARM64, x86_64
- 🛡️ **Type Safe** - Compile-time type checking
- 🚀 **Production Ready** - All tests passing
- 💎 **Extensible** - Extension methods on any type

## Language Features

### Variables & Types
```wyn
var x = 42              // Type inference
var name: str = "Wyn"   // Explicit types
const PI = 3.14159      // Constants
```

### Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str = "World") {  // Default parameters
    print("Hello, ${name}!")
}
```

### Control Flow
```wyn
// If/else
if x > 10 {
    print("Big")
} else {
    print("Small")
}

// Ternary
var result = x > 10 ? "Big" : "Small"

// Match
match value {
    1 => print("One")
    2 => print("Two")
    _ => print("Other")
}
```

### Collections
```wyn
// Arrays
var nums = [1, 2, 3, 4, 5]
var first = nums[0]
var slice = nums[1:4]

// List comprehensions
var evens = [x for x in nums if x % 2 == 0]

// HashMap
import collections
var map = collections.HashMap.new()
```

### Modules
```wyn
import fs
import compress

fn main() {
    var data = fs.read("file.txt")
    var compressed = compress.gzip(data)
    fs.write("file.gz", compressed)
}
```

### Concurrency
```wyn
fn worker(id: int) {
    print("Worker ${id}")
}

fn main() {
    spawn worker(1)
    spawn worker(2)
    spawn worker(3)
}
```

## Standard Library

Built-in modules:
- **fs** - File operations
- **net** - HTTP, TCP, UDP
- **os** - Environment, processes
- **time** - Timestamps, sleep
- **json** - JSON parsing
- **crypto** - Encryption
- **compress** - Gzip compression
- **collections** - HashMap, Set
- **string** - String utilities
- **array** - Array operations
- And more...

## Performance

Real-world benchmarks vs Python:

| Workload | Python | Wyn | Speedup |
|----------|--------|-----|---------|
| Log processing (1GB) | 15.0s | 2.5s | **6.0x** |
| JSON processing (10K) | 3.2s | 0.8s | **4.0x** |
| Concurrent API calls | 8.5s | 1.2s | **7.1x** |
| Data pipeline | 12.3s | 2.1s | **5.9x** |

**Why Wyn is faster:**
- Native compilation (LLVM-optimized)
- True parallelism (no GIL)
- Zero-cost abstractions

## Building

```bash
make          # Build compiler
make test     # Run tests
make clean    # Clean build
```

## Documentation

Visit [wynlang.com](https://wynlang.com) for tutorials, guides, and API documentation.

## License

MIT - See LICENSE file
