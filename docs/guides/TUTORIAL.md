# Wyn Programming Tutorial

Learn Wyn in 10 minutes!

## Installation

```bash
# Clone repository
git clone https://github.com/yourusername/wyn-lang
cd wyn-lang

# Build compiler (requires LLVM)
make

# Verify installation
./build/wync --version
```

## Hello World

```wyn
fn main() {
    print("Hello, World!")
}
```

Compile and run:
```bash
./build/wync hello.wyn
./a.out
```

## Variables

```wyn
fn main() {
    let x: int = 42
    let y: int = 10
    let name: str = "Wyn"
    
    print(x)
    print(y)
    print(name)
}
```

## Arithmetic

```wyn
fn main() {
    let x: int = 10
    let y: int = 5
    
    print(x + y)  // 15
    print(x - y)  // 5
    print(x * y)  // 50
    print(x / y)  // 2
    print(x % y)  // 0
}
```

## Control Flow

```wyn
fn main() {
    let x: int = 10
    
    if x > 5 {
        print("Greater than 5")
    } else {
        print("Less than or equal to 5")
    }
}
```

## Loops

### While Loop
```wyn
fn main() {
    let i: int = 0
    while i < 5 {
        print(i)
        i = i + 1
    }
}
```

### For Loop (Range)
```wyn
fn main() {
    // Exclusive range
    for i in 0..5 {
        print(i)  // 0 1 2 3 4
    }
    
    // Inclusive range
    for i in 0..=5 {
        print(i)  // 0 1 2 3 4 5
    }
}
```

### Break and Continue
```wyn
fn main() {
    for i in 0..10 {
        if i == 3 {
            continue  // Skip 3
        }
        if i == 7 {
            break  // Stop at 7
        }
        print(i)  // 0 1 2 4 5 6
    }
}
```

## Arrays

```wyn
fn main() {
    let arr: [int] = [10, 20, 30, 40, 50]
    
    print(arr[0])  // 10
    print(arr[1])  // 20
    print(arr[4])  // 50
}
```

## Functions

### Simple Function
```wyn
fn greet(name: str) {
    print("Hello, ")
    print(name)
}

fn main() {
    greet("World")
}
```

### Function with Return Value
```wyn
fn add(x: int, y: int) -> int {
    return x + y
}

fn main() {
    let result: int = add(5, 3)
    print(result)  // 8
}
```

### Recursive Function
```wyn
fn fibonacci(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

fn main() {
    print(fibonacci(0))   // 0
    print(fibonacci(1))   // 1
    print(fibonacci(5))   // 5
    print(fibonacci(10))  // 55
}
```

## Complete Example

```wyn
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn sum_range(start: int, end: int) -> int {
    let total: int = 0
    for i in start..end {
        total = total + i
    }
    return total
}

fn main() {
    // Variables
    let x: int = 5
    let y: int = 10
    
    // Arithmetic
    let sum: int = x + y
    print(sum)
    
    // Arrays
    let numbers: [int] = [1, 2, 3, 4, 5]
    print(numbers[0])
    print(numbers[4])
    
    // Functions
    print(factorial(5))      // 120
    print(sum_range(1, 11))  // 55
    
    // Loops
    for i in 0..3 {
        print(i)
    }
}
```

## Tips & Tricks

### Type Annotations
Always specify types explicitly:
```wyn
let x: int = 42      // ✓ Good
let x = 42           // ✗ Type inference not yet supported
```

### Print Multiple Values
Call print multiple times:
```wyn
print("x = ")
print(x)
print(", y = ")
print(y)
```

### Array Size
Arrays are fixed-size at creation:
```wyn
let arr: [int] = [1, 2, 3]  // Size is 3
```

## Common Errors

### Undefined Variable
```wyn
fn main() {
    print(x)  // Error: undefined variable 'x'
}
```
**Fix**: Declare variable first with `let x: int = 0`

### Type Mismatch
```wyn
fn main() {
    let x: int = "hello"  // Error: type mismatch
}
```
**Fix**: Use correct type: `let x: str = "hello"`

### Missing Return
```wyn
fn add(x: int, y: int) -> int {
    x + y  // Error: missing return
}
```
**Fix**: Add `return` keyword: `return x + y`

## Next Steps

1. Read more examples in `examples/`
2. Check `docs/WORKING_EXAMPLES.md` for tested examples
3. Try writing your own programs
4. Report issues on GitHub

## Advanced Topics

### Cross-Compilation
```bash
# Compile for Linux x86_64
./build/wync --target x86_64 --target-os linux program.wyn

# Compile for ARM64 Linux
./build/wync --target arm64 --target-os linux program.wyn
```

### Optimization
```bash
# No optimization (faster compile)
./build/wync -O0 program.wyn

# Full optimization (faster runtime)
./build/wync -O2 program.wyn
```

## Resources

- **Language Reference**: `docs/language.md`
- **Standard Library**: `docs/stdlib.md`
- **LLVM Backend**: `docs/LLVM_COMPLETE.md`
- **Examples**: `examples/`

## Get Help

- GitHub Issues: Report bugs
- GitHub Discussions: Ask questions
- Documentation: `docs/`

---

**Happy Coding with Wyn!** 🚀
