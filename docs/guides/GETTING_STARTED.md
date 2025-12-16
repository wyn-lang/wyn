# Getting Started with Wyn

Welcome to Wyn! This guide will get you up and running in 5 minutes.

## Installation

### Prerequisites
- macOS or Linux
- LLVM 14+ (for LLVM backend)
- GCC or Clang
- Make

### Install LLVM (macOS)
```bash
brew install llvm
```

### Install LLVM (Linux)
```bash
# Ubuntu/Debian
sudo apt install llvm-14 clang

# Fedora
sudo dnf install llvm clang
```

### Build Wyn
```bash
git clone https://github.com/yourusername/wyn-lang.git
cd wyn-lang
make
```

This builds the `wync` compiler in `build/wync`.

### Verify Installation
```bash
./build/wync --version
# Output: wyn 0.2.0
```

## Your First Program

### Hello World

Create `hello.wyn`:
```wyn
fn main() {
    print("Hello, Wyn!")
}
```

Compile and run:
```bash
./build/wync hello.wyn
./a.out
# Output: Hello, Wyn!
```

### Variables and Arithmetic

Create `calc.wyn`:
```wyn
fn main() {
    let x: int = 42
    let y: int = 10
    let sum: int = x + y
    
    print("Sum: ")
    print(sum)
}
```

Run it:
```bash
./build/wync calc.wyn && ./a.out
# Output: Sum: 52
```

### Functions

Create `functions.wyn`:
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn multiply(a: int, b: int) -> int {
    return a * b
}

fn main() {
    let result: int = add(10, 20)
    print("10 + 20 = ")
    print(result)
    
    let product: int = multiply(5, 6)
    print("5 * 6 = ")
    print(product)
}
```

## Core Concepts

### Variables

```wyn
// Mutable variable
let x: int = 10
x = 20  // OK

// Immutable variable
const y: int = 10
y = 20  // Error: cannot assign to const
```

### Types

```wyn
let i: int = 42          // Integer
let f: float = 3.14      // Float
let s: str = "hello"     // String
let b: bool = true       // Boolean
let arr: [int] = [1,2,3] // Array
```

### Control Flow

```wyn
// If/else
if x > 10 {
    print("Large")
} else {
    print("Small")
}

// While loop
let i: int = 0
while i < 5 {
    print(i)
    i = i + 1
}

// For loop
for i in 0..10 {
    print(i)
}
```

### Arrays

```wyn
let arr: [int] = [10, 20, 30, 40, 50]

// Indexing
print(arr[0])   // 10
print(arr[2])   // 30
print(arr[-1])  // 50 (negative indexing)

// Length
print(arr.len())  // 5

// Iteration
for x in arr {
    print(x)
}
```

### Structs

```wyn
struct Point {
    x: int
    y: int
}

fn main() {
    let p: Point = Point { x: 5, y: 10 }
    print(p.x)  // 5
    
    p.x = 15
    print(p.x)  // 15
}
```

### Enums and Match

```wyn
enum Status {
    Success,
    Error,
    Pending
}

fn main() {
    let status: int = 0  // Success
    
    match status {
        0 => print("Success")
        1 => print("Error")
        2 => print("Pending")
        _ => print("Unknown")
    }
}
```

## Concurrency

### Spawn Blocks

```wyn
fn main() {
    print("Main thread")
    
    // Spawn concurrent task
    spawn {
        print("Task 1")
    }
    
    spawn {
        print("Task 2")
    }
    
    print("Done spawning")
}
```

Output (concurrent):
```
Main thread
Done spawning
Task 1
Task 2
```

### Variable Capture

```wyn
fn main() {
    let counter: int = 0
    
    // Spawn 1000 tasks that increment counter
    for i in 0..1000 {
        spawn {
            counter = counter + 1  // Captures counter
        }
    }
    
    print("Spawned 1000 tasks")
}
```

### Async/Await

```wyn
async fn compute(x: int) -> int {
    return x * 2
}

fn main() {
    let result: int = await compute(21)
    print(result)  // 42
}
```

## Modules

### Using Stdlib

```wyn
import string

fn main() {
    let s: str = int_to_str(42)
    print(s)  // "42"
}
```

### Available Modules

- `string` - String manipulation
- `io` - File I/O
- `os` - OS operations
- `test` - Testing utilities
- `task` - Task channels
- `math` - Mathematical functions
- `time` - Time operations
- ...and 22 more!

See `std/` directory for all modules.

## Compilation

### Basic Usage

```bash
# Compile
./build/wync program.wyn

# Specify output
./build/wync -o myapp program.wyn

# Run
./myapp
```

### Optimization

```bash
# No optimization (default)
./build/wync -O0 program.wyn

# Basic optimization
./build/wync -O1 program.wyn

# Full optimization
./build/wync -O2 program.wyn
```

### Cross-Compilation

```bash
# Target x86_64 Linux
./build/wync --target x86_64 --target-os linux program.wyn

# Target ARM64 macOS
./build/wync --target arm64 --target-os macos program.wyn
```

## Common Patterns

### Fibonacci (Recursion)

```wyn
fn fib(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

fn main() {
    print(fib(10))  // 55
}
```

### Concurrent Counter

```wyn
fn main() {
    let counter: int = 0
    
    for i in 0..100000 {
        spawn {
            counter = counter + 1
        }
    }
    
    print("Spawned 100k tasks")
}
```

### Pattern Matching

```wyn
enum Result {
    Ok,
    Error
}

fn process(status: int) {
    match status {
        0 => print("Success!")
        1 => print("Failed!")
        _ => print("Unknown")
    }
}
```

## Debugging

### Compilation Errors

```bash
# Verbose output
./build/wync program.wyn

# Check LLVM IR
./build/wync --emit-ir program.wyn
cat /tmp/wyn_*.ll
```

### Runtime Debugging

```bash
# The M:N scheduler prints debug info
./a.out
# [Runtime] Initializing M:N scheduler...
# [Runtime] Using 12 worker threads
# [Runtime] M:N scheduler initialized
```

## Performance Tips

### Concurrency

```wyn
// ✅ Good: Spawn many lightweight tasks
for i in 0..1000000 {
    spawn { /* minimal work */ }
}

// ❌ Bad: Heavy work in each task
for i in 0..1000 {
    spawn { /* expensive computation */ }
}
```

### Arrays

```wyn
// ✅ Good: Use for loops
for i in 0..arr.len() {
    print(arr[i])
}

// ✅ Good: Direct iteration
for x in arr {
    print(x)
}
```

## Next Steps

### Learn More
- [Examples](EXAMPLES.md) - More code examples
- [Concurrency](CONCURRENCY.md) - Deep dive into spawn/async
- [M:N Scheduler](MN_SCHEDULER.md) - Runtime architecture

### Try Examples
```bash
# Run example programs
./build/wync examples/loops.wyn && ./a.out
./build/wync examples/spawn_demo.wyn && ./a.out
./build/wync examples/match_test.wyn && ./a.out
```

### Run Tests
```bash
make test
```

### Explore Stdlib
```bash
ls std/
# array.wyn  io.wyn  math.wyn  os.wyn  string.wyn  task.wyn  time.wyn  ...
```

## Getting Help

### Documentation
- `docs/` - Full documentation
- `examples/` - Working examples
- `STATUS.md` - Current status

### Common Issues

**Issue:** `undefined variable 'some_function'`  
**Solution:** Add `import module_name` at top of file

**Issue:** `LLVM compilation failed`  
**Solution:** Check for unsupported features (lambdas, generics)

**Issue:** Segmentation fault  
**Solution:** Report as bug (some edge cases still being fixed)

## Contributing

Wyn is under active development. Core features are stable.

**Areas needing work:**
- Result type handling
- Lambdas
- Generics
- Advanced stdlib modules

See [STATUS.md](../STATUS.md) for detailed status.

## Summary

You now know how to:
- ✅ Install and build Wyn
- ✅ Write basic programs
- ✅ Use variables, functions, arrays
- ✅ Use spawn for concurrency
- ✅ Import stdlib modules
- ✅ Compile and run programs

**Next:** Explore `examples/` directory for more advanced usage!

Welcome to Wyn! 🎉
