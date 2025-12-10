# Wyn Quick Reference

## Variables
```wyn
let x: int = 42        // Mutable
const y: int = 10      // Immutable
let z = 100            // Type inferred
```

## Types
```wyn
int     // 64-bit integer
float   // 64-bit float
str     // String
bool    // Boolean
[int]   // Array of integers
```

## Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

async fn fetch() -> int {
    return 42
}
```

## Control Flow
```wyn
// If/else
if x > 10 {
    print("yes")
} else {
    print("no")
}

// While
while i < 10 {
    i = i + 1
}

// For
for i in 0..10 {
    print(i)
}
```

## Arrays
```wyn
let arr: [int] = [1, 2, 3]
arr[0]      // 1
arr[-1]     // 3 (negative index)
arr.len()   // 3
```

## Structs
```wyn
struct Point {
    x: int
    y: int
}

let p: Point = Point { x: 5, y: 10 }
p.x         // 5
p.y = 20    // Mutation
```

## Enums
```wyn
enum Color {
    Red,
    Green,
    Blue
}

let c: int = 0  // Red
```

## Match
```wyn
match value {
    0 => print("zero")
    1 => print("one")
    _ => print("other")
}
```

## Concurrency
```wyn
// Spawn task
spawn {
    print("concurrent")
}

// Variable capture
let x: int = 42
spawn {
    print(x)  // Captures x
}

// Async/await
let result: int = await compute()
```

## Modules
```wyn
import string
import io
import os

let s: str = int_to_str(42)
```

## Operators
```wyn
// Arithmetic
+ - * / %

// Comparison
== != < > <= >=

// Logical
&& || !

// Range
0..10      // 0 to 9
0..=10     // 0 to 10
```

## Comments
```wyn
// Single line comment

# Also single line

/* Multi-line
   comment */
```

## Compilation
```bash
# Basic
./build/wync program.wyn

# With output
./build/wync -o app program.wyn

# Optimized
./build/wync -O2 program.wyn

# Cross-compile
./build/wync --target x86_64 program.wyn
```

## Common Patterns

### Fibonacci
```wyn
fn fib(n: int) -> int {
    if n <= 1 { return n }
    return fib(n-1) + fib(n-2)
}
```

### Counter
```wyn
let count: int = 0
for i in 0..100 {
    spawn { count = count + 1 }
}
```

### Array Sum
```wyn
let sum: int = 0
for x in arr {
    sum = sum + x
}
```

## Stdlib Functions

### String
```wyn
int_to_str(42)        // "42"
ord("A")              // 65
chr(65)               // "A"
```

### Array
```wyn
arr.len()             // Length
arr[i]                // Index
arr[-1]               // Last element
```

### IO
```wyn
import io
io.read_file("file.txt")
io.write_file("out.txt", "data")
```

### OS
```wyn
import os
os.exit(0)
os.args()
```

## Performance

### Concurrency
- 1.3M tasks/sec
- 1M+ concurrent tasks
- 8KB per task

### Compilation
- 0.3-0.5s typical
- LLVM-optimized
- Native code

## Tips

✅ **Do:**
- Use `let` for mutable, `const` for immutable
- Import modules for stdlib functions
- Use spawn for concurrent tasks
- Use match for pattern matching

❌ **Don't:**
- Call builtins directly (use modules)
- Use `mut` keyword (doesn't exist)
- Expect lambdas/generics (not implemented yet)

## Help

- `./build/wync --help` - Compiler options
- `make test` - Run test suite
- `docs/` - Full documentation
- `examples/` - Working examples
