# Wyn Language Reference

Complete language guide and API reference for Wyn v1.1.0.

---

## Table of Contents

1. [Syntax](#syntax)
2. [Types](#types)
3. [Functions](#functions)
4. [Structs & Methods](#structs--methods)
5. [Enums & Pattern Matching](#enums--pattern-matching)
6. [Generics](#generics)
7. [Standard Library](#standard-library)

---

## Syntax

### Variables
```wyn
var x = 42              // Type inference
var y: int = 100        // Explicit type
var name = "Alice"      // String
```

### Type Aliases
```wyn
type UserId = int
type Email = string

var id: UserId = 42
```

### Operators
```wyn
// Arithmetic
x + y, x - y, x * y, x / y, x % y

// Comparison
x == y, x != y, x < y, x > y, x <= y, x >= y

// Logical
x && y, x || y, !x

// Nil coalescing
value ?? default        // Returns value if Some, default if None
```

---

## Types

### Primitives
```wyn
int     // 64-bit integer
float   // 64-bit float
bool    // true/false
string  // UTF-8 string
```

### Collections
```wyn
// Arrays
var arr = [1, 2, 3]
arr.push(4)
var len = arr.len()

// Maps
var map = {"key": "value"}
map.insert("new", "data")
var val = map.get("key")
```

### Options
```wyn
var x = some(42)        // Option<int>
var y = none()          // Option<T>

// Nil coalescing
var result = x ?? 0     // Returns 42
var result2 = y ?? 100  // Returns 100

// Pattern matching
match x {
    some(val) => println("Got: {val}"),
    none() => println("Nothing")
}
```

---

## Functions

### Basic Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: string) {
    println("Hello, {name}!")
}
```

### Generic Functions
```wyn
fn identity<T>(value: T) -> T {
    return value
}

var x = identity(42)
var y = identity("hello")
```

---

## Structs & Methods

### Struct Definition
```wyn
struct Point {
    x: int,
    y: int
}

var p = Point { x: 3, y: 4 }
```

### Extension Methods (impl blocks)
```wyn
impl Point {
    fn sum(self) -> int {
        return self.x + self.y
    }
    
    fn distance(self) -> float {
        return ((self.x * self.x + self.y * self.y) as float).sqrt()
    }
}

var p = Point { x: 3, y: 4 }
var s = p.sum()         // Returns 7
var d = p.distance()    // Returns 5.0
```

---

## Enums & Pattern Matching

### Enum Definition
```wyn
enum Color {
    Red,
    Green,
    Blue,
    RGB(int, int, int)
}
```

### Pattern Matching
```wyn
fn describe(color: Color) -> string {
    match color {
        Red => return "Red",
        Green => return "Green",
        Blue => return "Blue",
        RGB(r, g, b) => return "RGB({r}, {g}, {b})"
    }
}
```

---

## Generics

### Generic Types
```wyn
enum Option<T> {
    Some(T),
    None
}

enum Result<T, E> {
    Ok(T),
    Err(E)
}
```

### Generic Structs
```wyn
struct Box<T> {
    value: T
}

var int_box = Box { value: 42 }
var str_box = Box { value: "hello" }
```

---

## Standard Library

### String Methods
```wyn
"hello".len()           // 5
"hello".upper()         // "HELLO"
"HELLO".lower()         // "hello"
"  hi  ".trim()         // "hi"
"hello".contains("ell") // true
"hello".starts_with("he") // true
"hello".ends_with("lo") // true
"hello".replace("l", "L") // "heLLo"
"a,b,c".split(",")      // ["a", "b", "c"]
```

### Integer Methods
```wyn
(-10).abs()             // 10
2.pow(3)                // 8
10.min(20)              // 10
10.max(5)               // 10
42.to_string()          // "42"
42.to_float()           // 42.0
```

### Float Methods
```wyn
16.0.sqrt()             // 4.0
3.7.round()             // 4.0
3.2.floor()             // 3.0
3.8.ceil()              // 4.0
(-5.5).abs()            // 5.5
3.14.to_int()           // 3
3.14.to_string()        // "3.14"
```

### Array Methods
```wyn
var arr = [1, 2, 3]
arr.len()               // 3
arr.push(4)             // [1, 2, 3, 4]
arr.pop()               // 4, arr is [1, 2, 3]
arr.get(0)              // Some(1)
arr.contains(2)         // true
arr.reverse()           // [3, 2, 1]
arr.sort()              // [1, 2, 3]
```

### Map Methods
```wyn
var map = {"a": 1, "b": 2}
map.len()               // 2
map.get("a")            // Some(1)
map.insert("c", 3)      // {"a": 1, "b": 2, "c": 3}
map.remove("b")         // {"a": 1, "c": 3}
map.contains_key("a")   // true
map.keys()              // ["a", "c"]
map.values()            // [1, 3]
```

### Control Flow
```wyn
// If expressions
var x = if condition { 10 } else { 20 }

// For loops
for i in 0..10 {
    println("{i}")
}

// While loops
while condition {
    // ...
}

// Match expressions
var result = match value {
    0 => "zero",
    1 => "one",
    _ => "other"
}
```

---

## Memory Management

Wyn uses Automatic Reference Counting (ARC):
- No manual memory management
- No garbage collection pauses
- Deterministic cleanup
- Cycle detection for circular references

---

## Async/Await

```wyn
async fn fetch_data() -> string {
    var result = await http_get("https://api.example.com")
    return result
}

fn main() -> int {
    var data = await fetch_data()
    println(data)
    return 0
}
```

---

## Building & Running

```bash
# Compile
./wyn program.wyn

# Run
./program.wyn.out

# One-liner
./wyn program.wyn && ./program.wyn.out
```

---

**Wyn v1.1.0 Language Reference**
