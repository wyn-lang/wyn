# Wyn Language - Working Examples

## Basic Features

### Variables and Arithmetic
```wyn
fn main() {
    let x: int = 42
    let y: int = 10
    let sum: int = x + y
    print(sum)  // 52
}
```

### Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    let result: int = add(10, 20)
    print(result)  // 30
}
```

### Recursion
```wyn
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn main() {
    print(factorial(5))  // 120
}
```

## Data Structures

### Arrays
```wyn
fn main() {
    let arr: [int] = [10, 20, 30, 40]
    print(arr[0])  // 10
    print(arr[2])  // 30
    
    for i in 0..4 {
        print(arr[i])
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
    let p: Point = Point { x: 5, y: 10 }
    print(p.x)  // 5
    print(p.y)  // 10
    
    p.x = 15
    print(p.x)  // 15
}
```

### Enums and Match
```wyn
enum Color {
    Red,
    Green,
    Blue
}

fn main() {
    let color: int = 1  // Green
    
    match color {
        0 => print("Red")
        1 => print("Green")
        2 => print("Blue")
        _ => print("Unknown")
    }
}
```

## Control Flow

### If/Else
```wyn
fn main() {
    let x: int = 42
    
    if x > 40 {
        print("Large")
    } else {
        print("Small")
    }
}
```

### While Loops
```wyn
fn main() {
    let i: int = 0
    while i < 5 {
        print(i)
        i = i + 1
    }
}
```

### For Loops
```wyn
fn main() {
    for i in 0..10 {
        print(i)
    }
    
    let arr: [int] = [10, 20, 30]
    for x in arr {
        print(x)
    }
}
```

## Advanced Features

### Floats
```wyn
fn main() {
    let pi: float = 3.14159
    let radius: float = 5.0
    let area: float = pi * radius * radius
    print(area)  // 78.539750
}
```

### Module Imports
```wyn
import string

fn main() {
    let s: str = int_to_str(42)
    print(s)  // "42"
    
    let code: int = ord("A")
    print(code)  // 65
}
```

### Spawn (Concurrency)
```wyn
fn main() {
    print("Main thread")
    
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

### Multiple Spawns
```wyn
fn main() {
    for i in 0..10 {
        spawn {
            print("Task running")
        }
    }
}
```

## Type System

### Type Inference
```wyn
fn main() {
    let x = 42        // Inferred as int
    let y = 3.14      // Inferred as float
    let s = "hello"   // Inferred as str
}
```

### Explicit Types
```wyn
fn main() {
    let x: int = 42
    let y: float = 3.14
    let s: str = "hello"
    let arr: [int] = [1, 2, 3]
}
```

## Performance

### Fibonacci (Recursion)
```wyn
fn fib(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

fn main() {
    print(fib(30))  // Fast native code
}
```

### Concurrent Tasks
```wyn
fn main() {
    // Spawn 100k tasks
    for i in 0..100000 {
        spawn {
            let x: int = i * 2
        }
    }
}
```

Performance: **1.3M tasks/sec**

## Compilation

### Basic
```bash
./build/wync program.wyn
./a.out
```

### With Output
```bash
./build/wync -o myapp program.wyn
./myapp
```

### Cross-Compile
```bash
./build/wync --target x86_64 --target-os linux program.wyn
```

## Notes

- All examples compile to native code via LLVM
- Type checking at compile time
- No runtime overhead
- Fast compilation (0.3-0.5s typical)
- M:N scheduler for concurrency (12 workers on test system)
