# Wyn Programming Language

**One language to rule them all.**

Wyn is a general-purpose programming language designed to be universal, fast, simple, and complete. Write servers, web apps, desktop GUIs, mobile apps, and systems code — all in one language.

## Goals

- **Universal**: Compile to native binaries for macOS, Linux, Windows, iOS, Android, and WebAssembly
- **Self-hosting**: The compiler compiles itself with zero external dependencies
- **Fast**: Near-C performance with modern ergonomics
- **Simple**: Python-like readability with Go-like simplicity
- **Concurrent**: Easy parallelism with explicit control
- **Complete**: Built-in support for GUI, TUI, networking, and mobile development

## Quick Start

```wyn
import std/io

fn main() {
    io.print("Hello, World!")
}
```

## Building

```bash
# Build the bootstrap compiler (Stage 0)
make stage0

# Compile a Wyn program
./build/stage0 -o hello examples/simple.wyn
./hello

# Generate assembly only
./build/stage0 -S -o output examples/simple.wyn

# See all options
make help
```

## Project Structure

```
wyn-lang/
├── SPEC.md              # Language specification
├── README.md            # This file
├── Makefile             # Build system
├── wyn.toml             # Project manifest
├── bootstrap/           # Bootstrap compiler (C)
│   └── stage0.c         # Stage 0 compiler
├── src/                 # Wyn compiler source (in Wyn)
│   └── compiler/
│       ├── lexer/       # Tokenizer
│       ├── parser/      # Parser & AST
│       ├── typechecker/ # Type checking
│       ├── ir/          # Intermediate representation
│       └── codegen/     # Code generation backends
├── examples/            # Example programs
└── tests/               # Test suite
```

## Status

**v0.1.0** - First Release (December 2025)

### What Works Now

The Stage 0 compiler (written in C) successfully compiles:
- ✅ **47 test programs** - All passing
- ✅ **500+ stdlib functions** - 12 modules (fs, net, http, json, math, crypto, regex, collections, time, log, and more)
- ✅ **Self-hosting ready** - Language features sufficient for Stage 1 compiler
- ✅ **Module system** - Clean namespace organization (io, fs, math, etc.)
- ✅ **Method syntax** - `.len()`, `.contains()`, `.index_of()` on primitives
- ✅ **10+ new builtins** - String operations, math functions, file system

### Core Language (SPEC Sections 1-2)
- [x] All primitive types (int, float, str, bool)
- [x] Arrays with literals, indexing, slicing, iteration
- [x] Tuples with unpacking
- [x] Structs with methods and nested access
- [x] Enums with integer values
- [x] Optional types (?T, some, none, ??, !)
- [x] Result types (Result[T,E], ok, err, ?, ??)
- [x] Generics for functions and structs
- [x] Functions with recursion, default params, lambdas
- [x] Control flow (if/else, while, for, match, break, continue)
- [x] Range syntax (0..10, 0..=10)
- [x] If expressions (if cond then a else b)
- [x] All operators (arithmetic, bitwise, comparison, logical, compound assignment)
- [x] defer for cleanup
- [x] Import system (stdlib and relative paths)
- [x] Comments (# and //)

### Partially Implemented
- ⚠️ **Maps** - Parsed, codegen incomplete
- ⚠️ **Interfaces** - Parsed, codegen incomplete
- ⚠️ **Match binding** - Literals work, variable binding (some(x) =>) needs codegen
- ⚠️ **String interpolation** - Lexer ready, codegen incomplete
- ⚠️ **Concurrency** - spawn/parallel parsed but run synchronously
- ⚠️ **pub visibility** - Parsed but not enforced

### Not Yet Implemented
- ❌ Enums with associated data (Color.Custom(r,g,b))
- ❌ try/catch error handling
- ❌ async/await, yield
- ❌ test/bench blocks
- ❌ @manual memory management
- ❌ Pointers (*Type)
- ❌ byte and any types
- ❌ Optional chaining (?.)
- ❌ List comprehensions
- ❌ GUI/TUI frameworks

See [SPEC_STATUS.md](SPEC_STATUS.md) for detailed feature tracking.

## Design Highlights

### Explicit Types

```wyn
let name: str = "Alice"
let age: int = 30
let score: float = 95.5

fn add(a: int, b: int) -> int {
    return a + b
}
```

### Optional Types (No Null)

```wyn
let maybe: ?int = some(42)

match maybe {
    some(n) => print("Got: {n}")
    none => print("Nothing")
}

let value: int = maybe ?? 0  // Default with ??
```

### Result Types for Errors

```wyn
fn read_file(path: str) -> Result[str, str] {
    // ...
}

let content: str = read_file("config.txt")?  // Propagate with ?
let safe: str = read_file("config.txt") ?? ""  // Default on error
```

### Easy Concurrency

```wyn
// Background task
spawn {
    do_work()
}

// Parallel loop
parallel for item in items {
    process(item)
}

// Thread-safe shared state
let counter: Shared[int] = shared(0)
counter.add(1)  // Atomic
```

### Generics

```wyn
struct Stack[T] {
    items: [T]
    
    fn push(mut self, item: T) {
        self.items.push(item)
    }
    
    fn pop(mut self) -> ?T {
        return self.items.pop()
    }
}
```

## Roadmap

### Stage 0 → Stage 1 (Q1 2026)
**Goal:** Self-hosting compiler written in Wyn

- [x] Complete Stage 0 bootstrap compiler (C)
- [x] 47 passing tests
- [x] 500+ stdlib functions
- [x] Module system (io, fs, math)
- [x] Method syntax (.len(), .contains(), .index_of())
- [x] 10+ new builtins (string ops, math, file system)
- [ ] Fix str_split bug
- [ ] String interpolation codegen
- [ ] Write Stage 1 compiler in Wyn
- [ ] Stage 1 compiles itself

### v0.2.0 (Q2 2026)
**Goal:** Production-ready core language

- [ ] Match with enum data binding
- [ ] Optional chaining (`?.`)
- [ ] byte and any types
- [ ] try/catch error handling
- [ ] test/bench blocks
- [ ] Interface implementation
- [ ] Map type codegen
- [ ] GUI/TUI frameworks (basic)

### v0.3.0 (Q3-Q4 2026)
**Goal:** Concurrency and multi-platform

- [ ] Real threading (spawn, parallel for)
- [ ] Shared[T] and Queue[T]
- [ ] async/await (basic)
- [ ] Linux and Windows support
- [ ] Optimization passes

### v1.0.0 (2027)
**Goal:** Universal platform support

- [ ] WebAssembly, iOS, Android
- [ ] @manual memory management
- [ ] Pointer types
- [ ] List comprehensions
- [ ] Package manager (full)
- [ ] Language server (LSP)

See [SPEC_STATUS.md](SPEC_STATUS.md) for detailed roadmap.

## License

MIT

## Contributing

Contributions welcome! See the [SPEC.md](SPEC.md) for language design details.
