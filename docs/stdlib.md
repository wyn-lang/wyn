# Wyn Standard Library Reference

The Wyn standard library is implemented in pure Wyn using syscalls - no C dependencies.

---

## Quick Start

```wyn
import io
import os
import string
import test

fn main() {
    # File I/O
    io.write_file("test.txt", "Hello")
    const data: str = io.read_file("test.txt")
    
    # String operations
    const upper: str = string.upper(data)
    
    # Testing
    test.assert(upper == "HELLO")
    
    # Exit
    os.exit(0)
}
```

---

## io - File I/O

**Import:** `import io`  
**Implementation:** Via syscalls (see [syscalls.md](syscalls.md))

### write_file()

```wyn
fn write_file(path: str, content: str) -> int
```

Write content to a file. Creates file if it doesn't exist, truncates if it does.

**Returns:** Number of bytes written, or negative on error

**Example:**

```wyn
import io

fn main() {
    const written: int = io.write_file("output.txt", "Hello, World!")
    
    if written < 0 {
        print("Error writing file")
    } else {
        print("Wrote", written, "bytes")
    }
}
```

### read_file()

```wyn
fn read_file(path: str) -> str
```

Read entire file contents as a string.

**Returns:** File contents, or empty string on error

**Example:**

```wyn
import io

fn main() {
    const content: str = io.read_file("input.txt")
    
    if content.len() == 0 {
        print("File empty or error")
    } else {
        print(content)
    }
}
```

### file_exists()

```wyn
fn file_exists(path: str) -> bool
```

Check if a file exists.

**Example:**

```wyn
import io

fn main() {
    if io.file_exists("config.txt") {
        const config: str = io.read_file("config.txt")
        print(config)
    } else {
        print("Config file not found")
    }
}
```

---

## os - Operating System

**Import:** `import os`  
**Implementation:** Via syscalls

### exit()

```wyn
fn exit(code: int)
```

Exit the program with the given exit code.

**Example:**

```wyn
import os

fn main() {
    print("Exiting...")
    os.exit(0)  # Success
}
```

### getpid()

```wyn
fn getpid() -> int
```

Get the current process ID.

**Example:**

```wyn
import os

fn main() {
    const pid: int = os.getpid()
    print("Process ID:", pid)
}
```

---

## string - String Utilities

**Import:** `import string`  
**Implementation:** Pure Wyn (no syscalls needed)

### from_int()

```wyn
fn from_int(n: int) -> str
```

Convert an integer to a string.

**Example:**

```wyn
import string

fn main() {
    const s: str = string.from_int(42)
    print(s)  # "42"
    
    const neg: str = string.from_int(-100)
    print(neg)  # "-100"
}
```

### upper()

```wyn
fn upper(s: str) -> str
```

Convert string to uppercase.

**Example:**

```wyn
import string

fn main() {
    const s: str = string.upper("hello")
    print(s)  # "HELLO"
}
```

### lower()

```wyn
fn lower(s: str) -> str
```

Convert string to lowercase.

**Example:**

```wyn
import string

fn main() {
    const s: str = string.lower("WORLD")
    print(s)  # "world"
}
```

### chr()

```wyn
fn chr(code: int) -> str
```

Convert ASCII code to a single-character string.

**Example:**

```wyn
import string

fn main() {
    const c: str = string.chr(65)
    print(c)  # "A"
    
    const newline: str = string.chr(10)
    print("Line 1" + newline + "Line 2")
}
```

**Note:** `ord()` is a builtin for getting character codes.

---

## test - Testing Utilities

**Import:** `import test`  
**Implementation:** Pure Wyn

### assert()

```wyn
fn assert(condition: bool)
```

Assert that a condition is true. Exits with code 1 if false.

**Example:**

```wyn
import test

fn main() {
    test.assert(2 + 2 == 4)
    test.assert(10 > 5)
    
    print("All assertions passed!")
}
```

### assert_eq()

```wyn
fn assert_eq(a: int, b: int)
```

Assert that two integers are equal. Exits with code 1 if not equal.

**Example:**

```wyn
import test

fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    test.assert_eq(add(2, 3), 5)
    test.assert_eq(add(10, -5), 5)
    
    print("All tests passed!")
}
```

---

## time - Time Functions

**Import:** `import time`  
**Implementation:** Via syscalls

### sleep_ms()

```wyn
fn sleep_ms(ms: int)
```

Sleep for the specified number of milliseconds.

**Example:**

```wyn
import time

fn main() {
    print("Waiting...")
    time.sleep_ms(1000)  # Sleep 1 second
    print("Done!")
}
```

### time_ms()

```wyn
fn time_ms() -> int
```

Get current time in milliseconds since epoch.

**Example:**

```wyn
import time

fn main() {
    const start: int = time.time_ms()
    
    # Do some work
    let sum: int = 0
    for i in 0..1000000 {
        sum = sum + i
    }
    
    const end: int = time.time_ms()
    const elapsed: int = end - start
    
    print("Elapsed:", elapsed, "ms")
}
```

---

## math - Mathematical Functions

**Import:** `import math`  
**Implementation:** Pure Wyn

### abs()

```wyn
fn abs(n: int) -> int
```

Return absolute value of an integer.

**Example:**

```wyn
import math

fn main() {
    const x: int = math.abs(-42)  # 42
    const y: int = math.abs(10)   # 10
}
```

### min()

```wyn
fn min(a: int, b: int) -> int
```

Return minimum of two integers.

**Example:**

```wyn
import math

fn main() {
    const x: int = math.min(5, 3)  # 3
}
```

### max()

```wyn
fn max(a: int, b: int) -> int
```

Return maximum of two integers.

**Example:**

```wyn
import math

fn main() {
    const x: int = math.max(5, 3)  # 5
}
```

### pow()

```wyn
fn pow(base: int, exp: int) -> int
```

Raise base to the power of exp.

**Example:**

```wyn
import math

fn main() {
    const x: int = math.pow(2, 8)   # 256
    const y: int = math.pow(10, 3)  # 1000
}
```

### sqrt()

```wyn
fn sqrt(n: float) -> float
```

Calculate square root using Newton's method.

**Example:**

```wyn
import math

fn main() {
    const x: float = math.sqrt(16.0)  # 4.0
    const y: float = math.sqrt(2.0)   # 1.414...
}
```

---

## array - Array Utilities

**Import:** `import array`  
**Implementation:** Pure Wyn

### sum()

```wyn
fn sum(arr: [int]) -> int
```

Sum all elements in an integer array.

**Example:**

```wyn
import array

fn main() {
    const numbers: [int] = [1, 2, 3, 4, 5]
    const total: int = array.sum(numbers)
    print(total)  # 15
}
```

### reverse()

```wyn
fn reverse(arr: [int]) -> [int]
```

Reverse an array.

**Example:**

```wyn
import array

fn main() {
    const numbers: [int] = [1, 2, 3, 4, 5]
    const reversed: [int] = array.reverse(numbers)
    # reversed = [5, 4, 3, 2, 1]
}
```

---

## collections - Data Structures

**Import:** `import collections`  
**Implementation:** Pure Wyn

Provides advanced data structures like hash maps, sets, queues, and stacks.

---

## http - HTTP Client/Server

**Import:** `import http`  
**Implementation:** Via syscalls

HTTP client and server functionality.

---

## json - JSON Parsing

**Import:** `import json`  
**Implementation:** Pure Wyn

JSON encoding and decoding.

---

## regex - Regular Expressions

**Import:** `import regex`  
**Implementation:** Pure Wyn

Pattern matching with regular expressions.

---

## Complete Module List

| Module | Description | Implementation |
|--------|-------------|----------------|
| `io` | File I/O | Syscalls |
| `os` | OS operations | Syscalls |
| `string` | String utilities | Pure Wyn |
| `test` | Testing | Pure Wyn |
| `time` | Time and sleep | Syscalls |
| `math` | Math functions | Pure Wyn |
| `array` | Array utilities | Pure Wyn |
| `collections` | Data structures | Pure Wyn |
| `bits` | Bit operations | Pure Wyn |
| `http` | HTTP client/server | Syscalls |
| `json` | JSON parsing | Pure Wyn |
| `regex` | Regular expressions | Pure Wyn |
| `crypto` | Cryptography | Pure Wyn |
| `hash` | Hash functions | Pure Wyn |
| `encoding` | Encoding/decoding | Pure Wyn |
| `compress` | Compression | Pure Wyn |
| `net` | Networking | Syscalls |
| `random` | Random numbers | Syscalls |
| `log` | Logging | Pure Wyn |
| `error` | Error handling | Pure Wyn |
| `validate` | Validation | Pure Wyn |
| `concurrent` | Concurrency | Pure Wyn |
| `tui` | Terminal UI | Syscalls |
| `gui` | GUI (future) | Platform-specific |
| `mobile` | Mobile (future) | Platform-specific |
| `gpu` | GPU computing | Platform-specific |
| `nn` | Neural networks | Pure Wyn |

---

## Implementation Philosophy

**Pure Wyn First:**
- Most modules implemented in pure Wyn
- No external dependencies
- Easy to understand and modify

**Syscalls for I/O:**
- File operations via syscalls
- Network operations via syscalls
- OS operations via syscalls
- See [syscalls.md](syscalls.md) for details

**No C Dependencies:**
- Entire stdlib in Wyn
- Portable across platforms
- Easy to audit and verify

---

## Learn More

- [Language Reference](language.md)
- [Syscalls Guide](syscalls.md)
- [Quick Start](quickstart.md)
