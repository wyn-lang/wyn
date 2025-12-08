# Getting Started with Wyn

## Installation

```bash
# Clone the repository
git clone https://github.com/wyn-lang/wyn.git
cd wyn-lang

# Build the compiler
make clean
make all

# Verify installation
./build/stage0 --version
```

## Your First Program

Create a file called `hello.wyn`:

```wyn
fn main() {
    print("Hello, World!")
}
```

Compile and run:

```bash
./build/stage0 -o hello hello.wyn
./hello
```

---

## Built-in Functions

**Wyn has only 5 builtins:**

### 1. print()

Output to stdout (the only I/O builtin):

```wyn
fn main() {
    print("Hello")
    print("Value:", 42)
    print("Pi:", 3.14)
}
```

### 2. args()

Get command line arguments:

```wyn
fn main() {
    const argv: [str] = args()
    
    print("Program:", argv[0])
    
    for i in 1..argv.len() {
        print("Arg", i, ":", argv[i])
    }
}
```

### 3. substring()

Extract substring (will become string method):

```wyn
fn main() {
    const s: str = "Hello, World!"
    const hello: str = substring(s, 0, 5)  # "Hello"
    const world: str = substring(s, 7, 12)  # "World"
    
    print(hello)
    print(world)
}
```

### 4. ord()

Get ASCII code of first character:

```wyn
fn main() {
    const code: int = ord("A")  # 65
    print("ASCII code of A:", code)
    
    const newline: int = ord("\n")  # 10
    print("ASCII code of newline:", newline)
}
```

### 5. syscall()

Direct system calls (see [syscalls.md](syscalls.md)):

```wyn
fn main() {
    # exit(0) - syscall 1 + 0x2000000 on macOS ARM64
    syscall(33554433, 0, 0, 0, 0, 0, 0)
}
```

**Everything else requires imports!**

---

## Using Imports

### File I/O

```wyn
import io

fn main() {
    # Write file
    const written: int = io.write_file("test.txt", "Hello, Wyn!")
    
    if written < 0 {
        print("Error writing file")
    } else {
        print("Wrote", written, "bytes")
    }
    
    # Read file
    const content: str = io.read_file("test.txt")
    print("Content:", content)
}
```

### String Operations

```wyn
import string

fn main() {
    # Convert int to string
    const s: str = string.from_int(42)
    print(s)  # "42"
    
    # Case conversion
    const upper: str = string.upper("hello")
    print(upper)  # "HELLO"
    
    # Character from code
    const c: str = string.chr(65)
    print(c)  # "A"
}
```

### Testing

```wyn
import test

fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    test.assert(add(2, 3) == 5)
    test.assert_eq(add(10, -5), 5)
    
    print("All tests passed!")
}
```

### OS Operations

```wyn
import os

fn main() {
    const pid: int = os.getpid()
    print("Process ID:", pid)
    
    # Exit with success code
    os.exit(0)
}
```

---

## Language Basics

### Variables

```wyn
fn main() {
    const x: int = 42           # Immutable
    let y: int = 0              # Mutable
    y = 10                      # OK
    
    const name: str = "Alice"
    const pi: float = 3.14
    const active: bool = true
}
```

### Functions

```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str) {
    print("Hello,", name, "!")
}

fn main() {
    const sum: int = add(5, 3)
    print("Sum:", sum)
    
    greet("Alice")
}
```

### Control Flow

```wyn
fn main() {
    # If/else
    const x: int = 10
    
    if x > 0 {
        print("positive")
    } else {
        print("non-positive")
    }
    
    # While loop
    let i: int = 0
    while i < 5 {
        print(i)
        i = i + 1
    }
    
    # For loop with range
    for i in 0..5 {
        print(i)
    }
}
```

### Arrays

```wyn
fn main() {
    const arr: [int] = [1, 2, 3, 4, 5]
    
    print(arr[0])        # First element: 1
    print(arr.len())     # Length: 5
    
    # Iterate
    for num in arr {
        print(num)
    }
}
```

### Structs

```wyn
struct Point {
    x: int
    y: int
}

fn main() {
    const p: Point = Point { x: 3, y: 4 }
    
    print("x:", p.x)
    print("y:", p.y)
}
```

---

## Example Programs

### Hello World

```wyn
fn main() {
    print("Hello, World!")
}
```

### File Processing

```wyn
import io
import string

fn main() {
    # Write data
    const data: str = "Line 1\nLine 2\nLine 3"
    io.write_file("data.txt", data)
    
    # Read and process
    const content: str = io.read_file("data.txt")
    const upper: str = string.upper(content)
    
    print(upper)
}
```

### Calculator

```wyn
import string
import test

fn add(a: int, b: int) -> int {
    return a + b
}

fn subtract(a: int, b: int) -> int {
    return a - b
}

fn multiply(a: int, b: int) -> int {
    return a * b
}

fn main() {
    # Test calculations
    test.assert_eq(add(5, 3), 8)
    test.assert_eq(subtract(10, 4), 6)
    test.assert_eq(multiply(6, 7), 42)
    
    print("All calculations correct!")
    
    # Display results
    const sum: int = add(100, 50)
    print("100 + 50 =", sum)
}
```

### Command Line Tool

```wyn
import io
import os

fn main() {
    const argv: [str] = args()
    
    if argv.len() < 2 {
        print("Usage:", argv[0], "<filename>")
        os.exit(1)
    }
    
    const filename: str = argv[1]
    
    if not io.file_exists(filename) {
        print("Error: File not found:", filename)
        os.exit(1)
    }
    
    const content: str = io.read_file(filename)
    print("File contents:")
    print(content)
    
    os.exit(0)
}
```

---

## Running Tests

```bash
# Run all tests
make test

# Run specific test
./build/stage0 -o test_output tests/test_file.wyn
./test_output
```

---

## Stage 1 Compiler

Wyn has a self-hosting compiler written in Wyn:

```bash
# Compile Stage 1 compiler
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# Use Stage 1 to compile programs
./build/stage1
./build/hello_from_stage1
```

---

## Next Steps

- Read the [Language Reference](language.md) for complete syntax
- Explore the [Standard Library](stdlib.md) for available modules
- Learn about [Syscalls](syscalls.md) for low-level programming
- Check out [examples/](../examples/) for more code samples

---

## Common Patterns

### Error Handling

```wyn
import io

fn main() {
    const written: int = io.write_file("test.txt", "data")
    
    if written < 0 {
        print("Error: Could not write file")
        # Handle error
    } else {
        print("Success: Wrote", written, "bytes")
    }
}
```

### String Building

```wyn
import string

fn main() {
    let result: str = ""
    
    for i in 1..6 {
        const num: str = string.from_int(i)
        result = result + num + " "
    }
    
    print(result)  # "1 2 3 4 5 "
}
```

### Testing Functions

```wyn
import test

fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn main() {
    test.assert_eq(factorial(0), 1)
    test.assert_eq(factorial(1), 1)
    test.assert_eq(factorial(5), 120)
    
    print("All tests passed!")
}
```

---

## Tips

1. **Use imports** - Only `print()` is global, everything else needs imports
2. **Check return values** - I/O functions return negative on error
3. **Test your code** - Use `import test` for assertions
4. **Read the docs** - See [language.md](language.md) for complete reference

---

## Getting Help

- Read the documentation in `docs/`
- Check examples in `examples/`
- Look at the standard library source in `std/`
- See the test suite in `tests/`
