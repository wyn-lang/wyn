# Getting Started with Wyn

## Installation

```bash
git clone https://github.com/wyn-lang/wyn.git
cd wyn
make stage0
```

## Your First Program

Create `hello.wyn`:
```wyn
fn main() {
    io.print_str("Hello, World!")
    io.print_newline()
}
```

Compile and run:
```bash
./build/stage0 -o hello hello.wyn
./hello
```

## Using Stage 1 (Recommended)

Stage 1 provides better errors and optimizations:

```bash
./build/stage0 --stage1-tc --stage1-opt -o hello hello.wyn
```

## Quick Examples

### Variables
```wyn
let x = 42          // Type inferred
let y: int = 100    // Explicit type
let mut z = 0       // Mutable
```

### Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

### Arrays
```wyn
let arr = [1, 2, 3, 4, 5]
print(arr[0])       // 1
print(len(arr))     // 5
```

### Control Flow
```wyn
if x > 10 {
    print_str("big")
} else {
    print_str("small")
}

while i < 10 {
    i = i + 1
}

for item in arr {
    print(item)
}
```

### Structs
```wyn
struct Point {
    x: int
    y: int
}

let p = Point { x: 10, y: 20 }
print(p.x)
```

## Performance

Wyn is fast:
- 20-30x faster than Python
- Similar to Java and Node.js
- 2-3x slower than C/Go
- Startup: 1.29ms (#2 fastest)

## Next Steps

- Read [Language Spec](2-language-spec.md)
- Check [Stdlib Reference](4-stdlib-reference.md)
- See [examples/](../examples/) for more code
- Run benchmarks: `cd benchmarks && ./run.sh`

## Getting Help

- Documentation: `docs/`
- Examples: `examples/`
- Tests: `tests/`
- GitHub: https://github.com/wyn-lang/wyn
