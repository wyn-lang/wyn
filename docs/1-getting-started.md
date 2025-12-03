# Getting Started with Wyn

## Installation

```bash
# Clone the repository
git clone https://github.com/your-repo/wyn-lang.git
cd wyn-lang

# Build the compiler
make stage0

# Verify installation
./build/stage0 --version
```

## Your First Program

Create a file called `hello.wyn`:

```wyn
fn main() {
    print_str("Hello, World!")
}
```

Compile and run:

```bash
./build/stage0 -o hello hello.wyn
./hello
```

## Language Basics

### Variables

```wyn
let x: int = 42           # Immutable
let mut y: int = 0        # Mutable
y = 10                    # OK
```

### Functions

```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str) {
    print_str("Hello, " + name + "!")
}
```

### Control Flow

```wyn
# If/else
if x > 0 {
    print_str("positive")
} else {
    print_str("non-positive")
}

# While loop
let mut i: int = 0
while i < 10 {
    print(i)
    i += 1
}

# For loop with range
for i in 0..10 {
    print(i)
}

# For loop over array
let arr: [int] = [1, 2, 3]
for item in arr {
    print(item)
}
```

### Arrays

```wyn
let arr: [int] = [1, 2, 3, 4, 5]
print(arr[0])        # First element
print(arr[-1])       # Last element
print(len(arr))      # Length

let slice: [int] = arr[1:3]  # [2, 3]
```

### Structs

```wyn
struct Point {
    x: int
    y: int
    
    fn distance_squared(self) -> int {
        return self.x * self.x + self.y * self.y
    }
}

fn main() {
    let p: Point = Point { x: 3, y: 4 }
    print(p.distance_squared())  # 25
}
```

### Optional Types

```wyn
fn find(arr: [int], target: int) -> ?int {
    for i in 0..len(arr) {
        if arr[i] == target {
            return some(i)
        }
    }
    return none
}

fn main() {
    let result: ?int = find([1, 2, 3], 2)
    let index: int = result ?? -1  # Default if none
}
```

### Generics

```wyn
struct Box[T] {
    value: T
}

fn identity[T](x: T) -> T {
    return x
}

fn main() {
    let b: Box[int] = Box[int] { value: 42 }
    let n: int = identity[int](10)
}
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `print(n)` | Print integer |
| `print_str(s)` | Print string |
| `len(x)` | Length of array or string |
| `abs(n)` | Absolute value |
| `min(a, b)` | Minimum of two values |
| `max(a, b)` | Maximum of two values |
| `ord(s)` | ASCII code of first char |
| `chr(n)` | String from ASCII code |
| `int_to_str(n)` | Convert int to string |
| `str_to_int(s)` | Convert string to int |
| `read_file(path)` | Read file contents |
| `assert(cond)` | Runtime assertion |
| `exit(code)` | Exit program |

## Running Tests

```bash
make test
```

## Next Steps

- Read the [Language Specification](SPEC.md)
- Explore [examples/](examples/)
- Check the [Standard Library](std/README.md)
