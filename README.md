# Wyn Programming Language

Fast, compiled systems language with clean, expressive syntax, powered by LLVM.

## What is Wyn?

Wyn is a **modern systems language** that combines:
- **Speed:** LLVM-powered native code generation
- **Simplicity:** Clean, expressive syntax
- **Multi-platform:** ARM64, x86_64, and more via LLVM
- **Concurrency:** Go-style green threads (1.3M tasks/sec)
- **Power:** Pattern matching, structs, enums, modules

## 🎉 Production Ready

The compiler is **production-ready** with **100% passing test suite**:
- ✅ Variables (let = mutable, const = immutable)
- ✅ Control flow (if/else, while, for, break, continue)
- ✅ Functions with parameters and recursion
- ✅ Arrays with indexing (including negative indices)
- ✅ Floats and strings
- ✅ Structs and enums
- ✅ Match statements (pattern matching)
- ✅ Spawn blocks (concurrent tasks)
- ✅ Async/await (basic)
- ✅ Module system (5 core stdlib modules)
- ✅ M:N Scheduler (1.3M tasks/sec)

## Test Suite Status

- **Examples**: 76/76 passing (100%)
- **Tests**: 15/15 passing (100%)
- **Total**: 91/91 passing (100%)

## Quick Start

```bash
# 1. Build compiler
make

# 2. Run tests (100% pass)
make test

# 3. Write program
cat > hello.wyn << 'EOF'
fn main() {
    print("Hello, Wyn!")
}
EOF

# 4. Compile and run
./build/wync hello.wyn
./a.out
```

## Example Programs

### Basic Features
```wyn
fn main() {
    let x: int = 42
    const y: int = 10
    print(x + y)
    
    let arr: [int] = [10, 20, 30]
    print(arr[0])
    print(arr[-1])
    
    for i in 0..5 {
        print(i)
    }
}
```

### Strings
```wyn
import string

fn main() {
    const s: str = "Hello World"
    print(s.len())
    
    const upper: str = string.upper(s)
    print(upper)
    
    const concat: str = "Hello" + " " + "Wyn"
    print(concat)
}
```

### Concurrency
```wyn
fn main() {
    let counter: int = 0
    
    for i in 0..1000 {
        spawn {
            counter = counter + 1
        }
    }
    
    print("Spawned 1000 tasks")
}
```

## Standard Library

### Core Modules (100% Working)
- **string** - String manipulation
- **array** - Array operations
- **math** - Mathematical functions
- **task** - Concurrency primitives
- **core** - Core types and utilities

### Future Modules (Archived)
See `archive/std/` for 24 modules planned for future releases.

## Performance

### Concurrency
- **1.3M tasks/sec** throughput
- **1M+ concurrent tasks** supported
- **8KB per task** (1000x better than pthread)

### Compilation
- **0.3-0.5s** for typical programs
- **100% type-safe** at compile time
- **LLVM-optimized** native code

## Building

```bash
make          # Build compiler
make test     # Run tests (100% pass)
make clean    # Clean build
make install  # Install to system
```

## Status

| Component | Status |
|-----------|--------|
| Core Language | ✅ 100% Complete |
| LLVM Backend | ✅ 100% Working |
| M:N Scheduler | ✅ Production Ready |
| Test Suite | ✅ 100% Passing |
| Core Stdlib | ✅ 5 Modules |

## Documentation

See [`docs/`](docs/) for detailed documentation.

## Contributing

Wyn has a clean, working baseline. See `archive/` for planned features.

## License

See LICENSE file.
