# Getting Started with Wyn

## What is Wyn?

Wyn is a **fast, compiled systems language with Python-like syntax**. It compiles to native code and runs at C-level speeds while being easy to write.

### Key Features
- **Fast:** Compiles to native ARM64/x86 code
- **Simple:** Python-like syntax, easy to learn
- **Self-hosting:** Compiler written in Wyn
- **Modern:** Traits, generics, pattern matching, async/await

## Quick Start (5 minutes)

### 1. Build the Compiler

```bash
cd wyn-lang
make stage0
```

This builds the bootstrap compiler in ~5 seconds.

### 2. Write Your First Program

Create `hello.wyn`:
```wyn
fn main() {
    print("Hello, Wyn!")
}
```

### 3. Compile and Run

```bash
./build/stage0 hello.wyn
./hello
```

That's it! Your program is compiled and running.

## Basic Syntax

### Variables
```wyn
let x: int = 5
const name: str = "Wyn"
let y = 10  # type inference
```

### Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str) {
    print("Hello, ", name)
}
```

### Control Flow
```wyn
# If statements
if x > 10 {
    print("Big")
} else if x > 5 {
    print("Medium")
} else {
    print("Small")
}

# While loops
let i = 0
while i < 5 {
    print(i)
    i = i + 1
}

# For loops
for i in 0..10 {
    print(i)
}
```

### Data Structures
```wyn
# Arrays
let numbers: [int] = [1, 2, 3, 4, 5]
print(numbers[0])

# Structs
struct Point {
    x: int
    y: int
}

let p = Point{x: 10, y: 20}
print(p.x)
```

### Pattern Matching
```wyn
match x {
    0 => print("Zero")
    1..10 => print("Small")
    _ => print("Large")
}
```

## Example Programs

### Fibonacci
```wyn
fn fib(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

fn main() {
    print(fib(10))  # 55
}
```

### FizzBuzz
```wyn
fn main() {
    for i in 1..101 {
        if i % 15 == 0 {
            print("FizzBuzz")
        } else if i % 3 == 0 {
            print("Fizz")
        } else if i % 5 == 0 {
            print("Buzz")
        } else {
            print(i)
        }
    }
}
```

### File I/O
```wyn
import io

fn main() {
    const data = io.read_file("input.txt")
    io.write_file("output.txt", data)
    print("Done!")
}
```

## Available Compilers

Wyn has multiple compilers for different use cases:

### wyncc - CLI Compiler
```bash
./build/stage0 -o build/wyncc src/stage1/wyncc.wyn
./build/wyncc your_program.wyn
```

### stage2_unified - Feature-Rich
```bash
./build/stage0 -o build/compiler src/stage1/stage2_unified.wyn
./build/compiler
```

## Standard Library

Import modules as needed:

```wyn
import io      # File I/O
import os      # Operating system
import string  # String utilities
import time    # Time and sleep
import math    # Math functions
```

Example:
```wyn
import io
import os

fn main() {
    const data = io.read_file("config.txt")
    print(data)
    os.exit(0)
}
```

## Advanced Features

### Traits
```wyn
trait Display {
    fn display(self)
}

impl Display for Point {
    fn display(self) {
        print("Point(", self.x, ", ", self.y, ")")
    }
}
```

### Generics
```wyn
fn swap<T>(a: T, b: T) -> (T, T) {
    return (b, a)
}

let (x, y) = swap(5, 10)
```

### Error Handling
```wyn
fn divide(a: int, b: int) -> (int, int) {
    if b == 0 {
        return (0, 1)  # error
    }
    return (a / b, 0)  # ok
}

let (result, err) = divide(10, 2)
if err == 0 {
    print("Result: ", result)
}
```

## Next Steps

### Learn More
- Read [Language Reference](language.md)
- Explore [Standard Library](stdlib.md)
- Check [Examples](../examples/)

### Build Something
- Start with simple programs
- Use the standard library
- Experiment with features

### Get Help
- Read the documentation
- Check example programs
- Review test files in `tests/`

## Tips

1. **Start Simple:** Begin with basic programs
2. **Use Examples:** Check `examples/` directory
3. **Read Tests:** Look at `tests/` for feature usage
4. **Experiment:** Try different features
5. **Build Fast:** Compilation is instant

## Common Patterns

### Reading Command-Line Arguments
```wyn
fn main() {
    const args: [str] = args()
    if len(args) > 1 {
        print("First arg: ", args[1])
    }
}
```

### Working with Files
```wyn
import io

fn main() {
    const lines = io.read_file("data.txt")
    io.write_file("output.txt", "Processed")
}
```

### Simple HTTP Server (Future)
```wyn
import http

fn main() {
    http.serve(8080, fn(req) {
        return "Hello, World!"
    })
}
```

## Why Wyn?

- **Fast:** Native compilation, C-level performance
- **Simple:** Python-like syntax, easy to learn
- **Modern:** All modern language features
- **Small:** Minimal runtime, small binaries
- **Self-hosting:** Compiler written in Wyn

## Summary

```bash
# 1. Build compiler
make stage0

# 2. Write program
echo 'fn main() { print("Hello!") }' > hello.wyn

# 3. Compile
./build/stage0 hello.wyn

# 4. Run
./hello
```

**Welcome to Wyn! Start coding in 5 minutes.** 🚀
