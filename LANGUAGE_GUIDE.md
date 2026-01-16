# Wyn Language Guide

Complete reference for the Wyn programming language.

## Table of Contents

1. [Basic Syntax](#basic-syntax)
2. [Types](#types)
3. [Functions](#functions)
4. [Control Flow](#control-flow)
5. [Structs](#structs)
6. [Generics](#generics)
7. [Async/Await](#asyncawait)
8. [Modules](#modules)
9. [Memory Management](#memory-management)

## Basic Syntax

### Comments
```wyn
// Single line comment
```

### Variables
```wyn
let x = 42;              // Immutable by default
let name = "Alice";      // String
let flag = true;         // Boolean
```

### Types
```wyn
let num: int = 42;
let pi: float = 3.14;
let text: string = "hello";
let flag: bool = true;
```

## Types

### Primitive Types
- `int` - Integer
- `float` - Floating point
- `bool` - Boolean
- `string` - String

### Composite Types
- `struct` - User-defined structures
- `array` - Fixed-size arrays
- `Optional<T>` - Optional values
- `Result<T>` - Result type for error handling

## Functions

### Basic Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b;
}
```

### No Return Value
```wyn
fn print_hello() -> void {
    print_str("Hello!");
}
```

### Multiple Parameters
```wyn
fn calculate(a: int, b: int, c: int) -> int {
    return a + b * c;
}
```

## Control Flow

### If Statements
```wyn
if x > 0 {
    print_str("Positive");
} else if x < 0 {
    print_str("Negative");
} else {
    print_str("Zero");
}
```

### While Loops
```wyn
let i = 0;
while i < 10 {
    print(i);
    i = i + 1;
}
```

### For Loops
```wyn
for i in 0..10 {
    print(i);
}
```

## Structs

### Definition
```wyn
struct Point {
    x: int,
    y: int
}
```

### Usage
```wyn
let p = Point { x: 10, y: 20 };
let x_val = p.x;
```

### Methods
```wyn
impl Point {
    fn distance(self) -> int {
        return self.x * self.x + self.y * self.y;
    }
}
```

## Generics

### Generic Functions
```wyn
fn identity<T>(x: T) -> T {
    return x;
}

fn main() -> int {
    let num = identity(42);
    return num;
}
```

### Generic Structs
```wyn
struct Box<T> {
    value: T
}
```

## Async/Await

### Async Functions
```wyn
async fn fetch_data() -> int {
    return 42;
}
```

### Await
```wyn
fn main() -> int {
    let future = fetch_data();
    let result = await future;
    return result;
}
```

### Multiple Async Operations
```wyn
async fn step1() -> int {
    return 10;
}

async fn step2(x: int) -> int {
    return x * 2;
}

fn main() -> int {
    let f1 = step1();
    let val = await f1;
    let f2 = step2(val);
    let result = await f2;
    return result;
}
```

## Modules

### Exporting
```wyn
// math.wyn
export fn add(a: int, b: int) -> int {
    return a + b;
}

export fn multiply(a: int, b: int) -> int {
    return a * b;
}
```

### Importing
```wyn
// main.wyn
import math;

fn main() -> int {
    return math::add(1, 2);
}
```

### Multiple Imports
```wyn
import math;
import utils;

fn main() -> int {
    let sum = math::add(1, 2);
    let product = utils::multiply(3, 4);
    return sum + product;
}
```

## Memory Management

### Automatic Reference Counting
```wyn
let data = Box::new(42);
// Automatically freed when out of scope
```

### Manual Memory Management
```wyn
let map = hashmap_new();
// ... use map ...
hashmap_free(map);
```

## Best Practices

1. **Use descriptive names** - `calculate_total` not `calc`
2. **Keep functions small** - One responsibility per function
3. **Handle errors** - Use Result type for fallible operations
4. **Document public APIs** - Add comments to exported functions
5. **Test your code** - Write tests for critical functionality

## Common Patterns

### Error Handling
```wyn
fn divide(a: int, b: int) -> int {
    if b == 0 {
        return -1;  // Error indicator
    }
    return a / b;
}
```

### Optional Values
```wyn
fn find_value(key: string) -> Optional<int> {
    // Return Some(value) or None
}
```

### Builder Pattern
```wyn
struct Config {
    host: string,
    port: int
}

fn new_config() -> Config {
    return Config { host: "localhost", port: 8080 };
}
```

## See Also

- [Standard Library Reference](STDLIB_REFERENCE.md)
- [Build Guide](BUILD_GUIDE.md)
- [Module Guide](MODULE_GUIDE.md)
