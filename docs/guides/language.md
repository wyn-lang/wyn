# Wyn Language Reference

**Version:** 0.1.0  
**Status:** Working Implementation

Wyn is a simple, fast programming language that compiles to native code.

---

## Philosophy

- **Simple**: Python-like syntax, easy to learn
- **Fast**: Compiles to native code, 20-30x faster than Python
- **Library-first**: Only 5 builtins, everything else via imports
- **Practical**: Built for real work - CLI tools, servers, scripts
- **Explicit**: Types are required, no surprises

---

## Quick Start

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

1. `print()` - Output to stdout
2. `args()` - Command line arguments (Stage 1 compiler needs)
3. `substring()` - String slicing (Stage 1 compiler needs)
4. `ord()` - Character code (Stage 1 compiler needs)
5. `syscall()` - Direct system calls (compiler intrinsic)

**Everything else requires imports:**

```wyn
import io
import os
import string
import test
import time
import math

fn main() {
    print("Hello!")  # Only builtin for I/O
    
    const data: str = io.read_file("file.txt")  # Via syscalls
    test.assert(data.len() > 0)  # Pure Wyn
    os.exit(0)  # Via syscalls
}
```

### print()

Output values to stdout:

```wyn
print("Hello")
print("Value:", 42)
print("Pi:", 3.14)
```

### args()

Get command line arguments:

```wyn
fn main() {
    const argv: [str] = args()
    for arg in argv {
        print(arg)
    }
}
```

### substring()

Extract substring (will become string method):

```wyn
const s: str = "Hello, World!"
const sub: str = substring(s, 0, 5)  # "Hello"
```

### ord()

Get ASCII code of first character:

```wyn
const code: int = ord("A")  # 65
const newline: int = ord("\n")  # 10
```

### syscall()

Direct system call (see [syscalls.md](syscalls.md)):

```wyn
# exit(0)
syscall(33554433, 0, 0, 0, 0, 0, 0)
```

---

## Types

### Primitives

```wyn
const name: str = "Alice"
const age: int = 30
const pi: float = 3.14
const active: bool = true
```

### Arrays

```wyn
# Declare
const numbers: [int] = [1, 2, 3, 4, 5]
let names: [str] = []

# Mutate
names.append("Alice")
names.append("Bob")

# Access
print(numbers[0])        # 1
print(numbers.len())     # 5

# Iterate
for num in numbers {
    print(num)
}
```

### Strings

```wyn
const greeting: str = "Hello"
const name: str = "World"

# Concatenation
const message: str = greeting + " " + name

# Interpolation (for strings only)
print("Hello, {name}!")

# Methods
print(name.len())           # 5
print(name.upper())         # WORLD
print(name.lower())         # world
```

---

## Functions

```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str) {
    print("Hello, {name}!")
}

fn main() {
    const sum: int = add(5, 3)
    print("Sum:", sum)
    greet("Alice")
}
```

---

## Structs

```wyn
struct User {
    name: str
    age: int
}

fn main() {
    const user: User = User{
        name: "Alice",
        age: 30
    }
    
    print(user.name)
    print(user.age)
}
```

### Struct Methods

```wyn
struct Point {
    x: int
    y: int
    
    fn distance(self) -> float {
        return sqrt(float(self.x * self.x + self.y * self.y))
    }
}

fn main() {
    const p: Point = Point{x: 3, y: 4}
    print(p.distance())  # 5.0
}
```

---

## Control Flow

### If/Else

```wyn
if age >= 18 {
    print("Adult")
} else {
    print("Minor")
}
```

### For Loops

```wyn
# Range
for i in 0..10 {
    print(i)
}

# Array
const names: [str] = ["Alice", "Bob", "Charlie"]
for name in names {
    print(name)
}
```

### While Loops

```wyn
let count: int = 0
while count < 10 {
    print(count)
    count = count + 1
}
```

---

## Concurrency

### Spawn

Run code in parallel:

```wyn
fn main() {
    let counter: int = 0
    
    for i in 0..100 {
        spawn {
            counter = counter + 1
        }
    }
    
    sleep_ms(1000)
    print(counter)  # 100
}
```

---

## Modules

### Import

```wyn
import io
import os
import time
import string

fn main() {
    # File I/O via syscalls
    io.write_file("test.txt", "Hello")
    const content: str = io.read_file("test.txt")
    print(content)
    
    # String utilities in pure Wyn
    const upper: str = string.upper("hello")
    const num: str = string.from_int(42)
    
    # OS operations via syscalls
    os.exit(0)
}
```

### Standard Library

All functionality via imports (see [stdlib.md](stdlib.md)):

- `io` - File I/O via syscalls
- `os` - OS operations via syscalls
- `string` - String utilities (pure Wyn)
- `test` - Testing utilities (pure Wyn)
- `time` - Time and sleep via syscalls
- `math` - Mathematical functions (pure Wyn)
- `array` - Array utilities (pure Wyn)
- `collections` - Data structures (pure Wyn)
- `http` - HTTP client/server
- `json` - JSON parsing
- `regex` - Regular expressions
- And 20+ more modules

---

## File I/O

```wyn
import io

fn main() {
    # Write file (returns bytes written or negative on error)
    const written: int = io.write_file("data.txt", "Hello, World!")
    
    # Read file
    const content: str = io.read_file("data.txt")
    print(content)
    
    # Check existence
    if io.file_exists("data.txt") {
        print("File exists")
    }
}
```

All I/O operations use syscalls directly - no C dependencies.

---

## System Operations

```wyn
import os

fn main() {
    # Get process ID
    const pid: int = os.getpid()
    print("PID:", pid)
    
    # Exit with code
    os.exit(0)
}
```

---

## String Operations

```wyn
import string

fn main() {
    # Convert int to string
    const s: str = string.from_int(42)
    
    # Case conversion
    const upper: str = string.upper("hello")
    const lower: str = string.lower("WORLD")
    
    # Character operations
    const code: int = ord("A")  # 65 (builtin)
    const c: str = string.chr(65)  # "A"
}
```

---

## Testing

```wyn
import test

fn main() {
    test.assert(2 + 2 == 4)
    test.assert_eq(10, 5 + 5)
    
    print("All tests passed!")
}
```

---

## HTTP

```wyn
import http

fn main() {
    # Server
    const server: int = http.serve("0.0.0.0", 8080)
    print("Server running on :8080")
    
    # Client
    const response: str = http.get("https://api.example.com")
    print(response)
}
```

---

## Examples

### CLI Calculator

```wyn
fn main() {
    print("Enter two numbers:")
    const a: int = str_to_int(input())
    const b: int = str_to_int(input())
    const sum: int = a + b
    print("Sum:", sum)
}
```

### File Processing

```wyn
import io

fn main() {
    const lines: [str] = io.read_lines("input.txt")
    let count: int = 0
    
    for line in lines {
        if line.len() > 0 {
            count = count + 1
        }
    }
    
    print("Lines:", count)
}
```

### HTTP Server

```wyn
import http

fn main() {
    const server: int = http.serve("0.0.0.0", 8080)
    print("Server running on :8080")
    
    while true {
        const req: Request = http.accept(server)
        spawn {
            http.respond(req, "Hello, World!")
        }
    }
}
```

---

## Compilation

```bash
# Basic compilation
wyn -o output input.wyn

# With optimizations (recommended)
wyn --stage1-tc --stage1-opt -o output input.wyn

# Run directly
wyn run input.wyn
```

---

## What's Next?

See [ROADMAP.md](ROADMAP.md) for planned features including:
- Generics
- Interfaces/Traits
- Pattern matching
- Error handling (Result/Option types)
- Async/await
- GPU computing

---

## Learn More

- [Quick Start Guide](quickstart.md)
- [Standard Library Reference](stdlib.md)
- [Syscalls Guide](syscalls.md)
- [Package Manager](packages.md)
