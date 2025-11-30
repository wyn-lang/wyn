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
fn main() {
    print("Hello, World!")
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

**v0.1.0** - First Release

- [x] Language specification (v0.2.0-draft)
- [x] Lexer (Stage 0 - C)
- [x] Parser (Stage 0 - C)
- [x] Type checker (Stage 0 - C)
- [x] ARM64 code generation (macOS)
- [x] x86_64 code generation (macOS)
- [x] Test suite (31 tests)
- [x] Standard library (partial)
- [ ] Self-hosting (Stage 1)
- [ ] Package manager
- [ ] GUI framework
- [ ] Mobile support

## Currently Supported

The Stage 0 compiler can compile programs with:
- Functions with parameters and return values
- Recursion
- Variables (`let`, `let mut`)
- Arithmetic operators (`+`, `-`, `*`, `/`, `%`, `**`)
- Bitwise operators (`|`, `&`, `^`, `~`, `<<`, `>>`)
- Comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`)
- Logical operators (`and`, `or`, `not`)
- Control flow (`if`/`else`, `while`, `for` loops)
- Range syntax in for loops (`for i in 0..10`, `for i in 0..=10` inclusive)
- `break` and `continue` in loops
- `match` statements with pattern matching
- `if` expressions (`if cond then a else b`)
- Function calls
- String literals and `print_str()` for string output
- String concatenation (`"hello" + "world"`)
- String comparison (`==`, `!=`)
- `print()` for integer output
- `assert()` for runtime assertions
- `exit()` for program termination
- `abs()`, `min()`, `max()` math builtins
- `int_to_str()`, `str_to_int()` type conversions
- `ord()`, `chr()` character code conversions
- `read_file()` for reading files to strings
- Method syntax on primitives (`.len()`, `.abs()`, `.to_str()`, `.to_int()`)
- Array literals and indexing (`[1, 2, 3]`, `arr[i]`)
- Array slicing (`arr[1:3]`, `arr[:2]`, `arr[3:]`, `arr[-2:]`)
- Array iteration in for loops (`for item in arr`)
- `len()` for array and string length
- Structs with field access (`Point { x: 1, y: 2 }`, `p.x`)
- Nested struct field access (`rect.origin.x`)
- Struct methods (`obj.method(args)`)
- Enums with integer values (`enum Color { Red, Green, Blue }`)
- Optional types (`?int`, `some(x)`, `none`, `x!`, `x ?? default`)
- Result types (`Result[T, E]`, `ok(x)`, `err(e)`, `x!`, `x ?? default`)
- Generics (`struct Box[T]`, `fn identity[T](x: T) -> T`, `Box[int]{...}`, `identity[int](42)`)
- Float type (basic support)
- Import system (`import math`)
- Comments (`#` and `//`)
- Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`)

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

## License

MIT

## Contributing

Contributions welcome! See the [SPEC.md](SPEC.md) for language design details.
