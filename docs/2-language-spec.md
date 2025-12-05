# Wyn Programming Language Specification

**Version:** 0.2.0-draft  
**Status:** Initial Design  
**Goal:** One language to rule them all — servers, web, desktop, mobile, systems, and everything in between.

---

## 1. Executive Summary

Wyn is a general-purpose programming language designed to be:
- **Universal**: Compile to native binaries for macOS, Linux, Windows, iOS, Android, and WebAssembly
- **Self-hosting**: The compiler compiles itself with zero external dependencies
- **Fast**: Near-C performance with modern ergonomics
- **Simple**: Python-like simplicity — if Python can do it simply, so should Wyn
- **Concurrent**: Easy parallelism with explicit control
- **Complete**: Built-in support for GUI, TUI, networking, and mobile development

### Design Philosophy

```
Simple > Everything else
Python's ease + Go's speed
Explicit > Implicit
One obvious way to do things
```

---

## 2. Core Language Design

### 2.1 Syntax Overview

Wyn looks and feels like Python, but compiles to native code:

```wyn
// Hello World
import std/io

fn main() {
    io.print("Hello, World!")
}

// Variables - types required
let name: str = "Wyn"
let age: int = 1
mut counter: int = 0

// Functions - types required
fn add(a: int, b: int) -> int {
    return a + b
}

// Structs with methods
struct User {
    name: str
    age: int
    
    fn greet(self) -> str {
        return "Hello, {self.name}!"
    }
}

// Pattern matching
match result {
    Ok(value) => print("Got: {value}")
    Err(e) => print("Error: {e}")
}
```

### 2.2 Module System

Wyn follows the principle: **"One obvious way to do things"**

All functionality is organized into modules. No top-level functions except language keywords.

#### Reserved Keywords (43 total)

Wyn has 43 reserved keywords organized by category:

**Declarations (8):**
`fn`, `struct`, `enum`, `interface`, `impl`, `let`, `mut`, `pub`

**Control Flow (9):**
`if`, `else`, `then`, `match`, `for`, `while`, `in`, `break`, `continue`, `return`

**Modules (3):**
`import`, `from`, `as`

**Literals (7):**
`true`, `false`, `none`, `some`, `ok`, `err`, `self`, `Self`

**Error Handling (3):**
`try`, `catch`, `defer`

**Concurrency (5):**
`spawn`, `parallel`, `async`, `await`, `yield`

**Operators (3):**
`and`, `or`, `not`

**Testing (3):**
`test`, `bench`, `assert`

**Note:** `assert` is a language keyword (like Python/Rust), not a stdlib function. It's used for runtime assertions and is always available without imports.

#### Standard Library Modules

```wyn
import std/io
import std/io
import std/math
import std/string

fn main() {
    // I/O operations
    io.print("Hello")
    io.println("World")
    let input: str = io.read_line()
    
    // File operations
    let content: str = fs.read_file("data.txt")
    fs.write_file("out.txt", content)
    let size: int = fs.file_size("data.txt")
    
    // Math operations
    let x: float = math.sin(1.0)
    let y: float = math.sqrt(4.0)
    
    // String methods (on the object)
    let len: int = "hello".len()
    let has: bool = "hello".contains("ell")
    let idx: int = "hello".index_of("l")
}
```

#### Module Organization

| Module | Purpose | Examples |
|--------|---------|----------|
| `std/io` | Input/output | `print`, `println`, `read_line`, `flush` |
| `std/io` | File system | `read_file`, `write_file`, `file_exists`, `mkdir` |
| `std/math` | Mathematics | `sin`, `cos`, `sqrt`, `pow`, `abs` |
| `std/string` | String utilities | Helper functions (most use methods) |
| `std/array` | Array utilities | `sum`, `max`, `min`, `sort` |
| `std/net` | Networking | `tcp_connect`, `tcp_send`, `tcp_recv` |
| `std/http` | HTTP client/server | `get`, `post`, `serve` |
| `std/json` | JSON encoding | `parse`, `stringify` |
| `std/time` | Time operations | `now`, `sleep`, `format` |
| `std/os` | OS operations | `getenv`, `system`, `getcwd` |

#### Methods vs Functions

**Methods** - operations ON an object:
```wyn
"hello".len()           // Get length
"hello".contains("l")   // Check substring
[1,2,3].len()          // Array length
x.abs()                // Absolute value
```

**Module Functions** - operations WITH objects:
```wyn
io.print("hello")       // Print to stdout
fs.read_file("x")       // Read file
math.sin(1.0)          // Calculate sine
```

**Rule:** If it's a property or transformation of the object, use a method. If it's an action involving the object, use a module function.

### 2.3 Type System

#### Primitive Types

6 core types:

| Type | Description | Example |
|------|-------------|---------|
| `int` | 64-bit signed integer (wraps on overflow) | `42`, `-17` |
| `float` | 64-bit floating point | `3.14`, `-0.5` |
| `str` | UTF-8 string (always valid UTF-8) | `"hello"` |
| `bool` | Boolean | `true`, `false` |
| `byte` | Unsigned 8-bit (for binary data) | `0xFF` |
| `any` | Dynamic type (runtime checked) | any value |

```wyn
let x: int = 42
let y: float = 3.14
let z: float = float(x) + y    // explicit conversion: 45.14
let w: int = x / 3             // integer division: 14
let v: float = float(x) / 3.0  // float division: 14.0
```

#### Arrays

Arrays are homogeneous and mutable by default:

```wyn
let numbers: [int] = [1, 2, 3, 4, 5]
let names: [str] = ["Alice", "Bob", "Charlie"]

// Access
print(numbers[0])      // 1
print(names[1])        // "Bob"

// Mutation (arrays are reference types, always mutable)
numbers.push(6)
numbers.pop()
let length: int = numbers.len()

// Slicing - Python style (creates a copy)
let first_two: [int] = numbers[0:2]    // [1, 2]
let last_two: [int] = numbers[-2:]     // [4, 5]
let reversed: [int] = numbers[::-1]    // [5, 4, 3, 2, 1]
let middle: [int] = numbers[1:-1]      // [2, 3, 4]

// Mixed types when needed (explicit)
let mixed: [any] = [1, "two", 3.0, true]
```

#### Tuples

Tuples are fixed-size, can be heterogeneous:

```wyn
let point: (int, int) = (10, 20)
let mixed: (int, str, bool) = (1, "hello", true)

// Access
print(point.0)    // 10
print(point.1)    // 20

// Unpacking
let (x, y): (int, int) = point
let (a, b, c): (int, str, bool) = mixed
```

#### Maps

Maps have explicit key and value types:

```wyn
let user: {str: any} = {
    "name": "Alice",
    "age": 30,
    "active": true
}

// Typed maps
let scores: {str: int} = {
    "alice": 100,
    "bob": 85
}

// Access
print(scores["alice"])     // 100
print(scores.get("bob"))   // 85

// Add/modify
scores["charlie"] = 90
```

#### Optional Types

Use `?Type` for values that might not exist:

```wyn
let maybe: ?int = some(42)
let nothing: ?int = none

match maybe {
    some(n) => print("Got: {n}")
    none => print("Nothing")
}

// Default with ??
let value: int = maybe ?? 0

// Check and use
if maybe {
    print("Has value: {maybe!}")  // ! unwraps
}

// Functions that might not return
fn find_user(id: int) -> ?User {
    if id in database {
        return some(database[id])
    }
    return none
}

let user: ?User = find_user(123)
let name: str = user?.name ?? "Anonymous"
```

#### Enums

```wyn
enum Color {
    Red
    Green
    Blue
    Custom(int, int, int)  // with associated data
}

let c: Color = Color.Red
let custom: Color = Color.Custom(255, 128, 0)

match c {
    Color.Red => print("Red!")
    Color.Green => print("Green!")
    Color.Blue => print("Blue!")
    Color.Custom(r, g, b) => print("RGB: {r}, {g}, {b}")
}

// Enums with values
enum Status {
    Ok = 200
    NotFound = 404
    Error = 500
}

let code: int = Status.Ok.value  // 200
```

#### Generics

Use `[T]` for generic type parameters:

```wyn
// Generic function
fn first[T](items: [T]) -> ?T {
    if items.len() > 0 {
        return some(items[0])
    }
    return none
}

// Generic struct
struct Stack[T] {
    items: [T]
    
    fn new() -> Stack[T] {
        return Stack[T]{items: []}
    }
    
    fn push(mut self, item: T) {
        self.items.push(item)
    }
    
    fn pop(mut self) -> ?T {
        return self.items.pop()
    }
}

let stack: Stack[int] = Stack[int].new()
stack.push(1)
stack.push(2)
let top: ?int = stack.pop()  // some(2)

// Generic with constraints
fn compare[T: Comparable](a: T, b: T) -> int {
    return a.compare(b)
}

// Result type (built-in)
enum Result[T, E] {
    Ok(T)
    Err(E)
}

fn divide(a: int, b: int) -> Result[int, str] {
    if b == 0 {
        return Err("division by zero")
    }
    return Ok(a / b)
}
```

#### Interfaces

Define behavior contracts with `interface`:

```wyn
interface Comparable {
    fn compare(self, other: Self) -> int
}

interface Printable {
    fn to_string(self) -> str
}

// Implement for a type
struct Point {
    x: int
    y: int
}

impl Comparable for Point {
    fn compare(self, other: Point) -> int {
        let d1: int = self.x + self.y
        let d2: int = other.x + other.y
        return d1 - d2
    }
}

impl Printable for Point {
    fn to_string(self) -> str {
        return "({self.x}, {self.y})"
    }
}

// Use in generic constraints
fn max[T: Comparable](a: T, b: T) -> T {
    return if a.compare(b) > 0 then a else b
}

// Multiple constraints
fn print_max[T: Comparable + Printable](a: T, b: T) {
    let m: T = max(a, b)
    print(m.to_string())
}
```

### 2.3 Type Conversion

Explicit conversion using type as function:

```wyn
let x: int = 42
let s: str = str(x)           // "42"
let f: float = float(x)       // 42.0
let i: int = int(3.14)        // 3 (truncates)
let i2: int = int("123")      // 123
let b: bool = bool(1)         // true
let raw: [byte] = x.to_bytes() // raw bytes

// Arrays to tuples
let arr: [int] = [1, 2, 3]
let t: (int, int, int) = tuple(arr)
```

### 2.4 Visibility

By default, everything is private to the module. Use `pub` to export:

```wyn
// Private - only accessible within this module
struct InternalHelper {
    data: str
}

fn helper_function() -> int {
    return 42
}

// Public - accessible from other modules
pub struct User {
    pub name: str      // public field
    pub age: int       // public field
    password: str      // private field
    
    pub fn new(name: str, age: int) -> User {
        return User{name: name, age: age, password: ""}
    }
    
    fn validate(self) -> bool {  // private method
        return self.password.len() > 8
    }
}

pub fn create_user(name: str) -> User {
    return User.new(name, 0)
}
```

### 2.5 Error Handling

Multiple approaches for different situations:

```wyn
// Option 1: Try/catch for exceptional cases
try {
    let content: str = read_file("config.txt")
    process(content)
} catch e {
    print("Error: {e}")
}

// Option 2: Result type for expected errors
fn read_config(path: str) -> Result[Config, str] {
    let content: Result[str, str] = fs.read(path)
    match content {
        Ok(data) => return parse_config(data)
        Err(e) => return Err("Failed to read: {e}")
    }
}

// Option 3: Propagate with ?
fn load_config() -> Result[Config, str] {
    let content: str = read_file("config.txt")?
    let config: Config = parse(content)?
    return Ok(config)
}

// Option 4: Default on error with ??
let content: str = read_file("config.txt") ?? ""
```

### 2.6 Control Flow

```wyn
// If/else
if x > 0 {
    print("positive")
} else if x < 0 {
    print("negative")
} else {
    print("zero")
}

// While loop
mut i: int = 0
while i < 10 {
    print(i)
    i += 1
}

// For loop (iterating)
for item in items {
    print(item)
}

// For loop (range)
for i in 0..10 {
    print(i)  // 0 to 9
}

for i in 0..=10 {
    print(i)  // 0 to 10 (inclusive)
}

// Break and continue
for i in 0..100 {
    if i == 50 {
        break
    }
    if i % 2 == 0 {
        continue
    }
    print(i)
}

// Match
match value {
    1 => print("one")
    2 | 3 => print("two or three")
    4..10 => print("four to nine")
    _ => print("other")
}
```

### 2.7 Functions

```wyn
// Basic function
fn add(a: int, b: int) -> int {
    return a + b
}

// No return value
fn greet(name: str) {
    print("Hello, {name}!")
}

// Default parameters
fn connect(host: str, port: int = 8080) {
    print("Connecting to {host}:{port}")
}
connect("localhost")        // uses port 8080
connect("localhost", 3000)  // uses port 3000

// Variadic functions
fn sum(numbers: ...int) -> int {
    mut total: int = 0
    for n in numbers {
        total += n
    }
    return total
}
sum(1, 2, 3, 4, 5)  // 15

// Anonymous functions (lambdas)
let double: fn(int) -> int = fn(x: int) -> int { return x * 2 }
let nums: [int] = [1, 2, 3].map(fn(x: int) -> int { return x * 2 })
```

### 2.8 Memory Management

Automatic garbage collection handles 99% of cases:

```wyn
let user: User = User{name: "Alice", age: 30}
// Memory is managed automatically
// No manual free, no reference counting to think about
```

For performance-critical code, opt into manual management with `@manual` blocks:

```wyn
@manual
fn fast_algorithm(data: [byte]) -> [byte] {
    let buffer: *byte = alloc(1024)  // raw pointer
    defer free(buffer)
    
    // Pointer arithmetic: buffer + offset
    // Dereference: buffer.*
    
    return result
}
```

Note: Pointers (`*Type`) are only available inside `@manual` blocks.

### 2.9 Optional vs Result

Use `?Type` when a value might not exist (no error info needed):
```wyn
fn find(id: int) -> ?User {
    return if id in db then some(db[id]) else none
}
let user: User = find(123) ?? default_user
```

Use `Result[T, E]` when you need error information:
```wyn
fn parse(s: str) -> Result[int, str] {
    // ... returns Ok(value) or Err("reason")
}
let value: int = parse("42")?  // propagates error
```

Both support `??` for defaults:
```wyn
let a: int = optional_value ?? 0
let b: int = result_value ?? 0  // uses default if Err
```

---

## 3. Concurrency Model

### 3.1 Spawn for Background Tasks

```wyn
// Run in background
spawn {
    do_work()
}

// Run and get result later
let task: Task[int] = spawn {
    return expensive_computation()
}
// ... do other stuff ...
let result: int = task.wait()

// Run multiple and wait for all
let tasks: [Task[Result]] = items.map(fn(item: Item) -> Task[Result] {
    return spawn { return process(item) }
})
let results: [Result] = wait_all(tasks)
```

### 3.2 Parallel Loops

Use `parallel for` for explicit parallelism:

```wyn
// Sequential (default)
for item in items {
    process(item)
}

// Parallel - items processed concurrently
parallel for item in items {
    process(item)
}

// Parallel map
let results: [Output] = parallel items.map(fn(x: Input) -> Output {
    return expensive(x)
})
```

### 3.3 Shared State

Thread-safe primitives with atomic operations:

```wyn
let counter: Shared[int] = shared(0)

spawn { counter.add(1) }      // atomic increment
spawn { counter.add(1) }

// Atomic update with function
counter.update(fn(x: int) -> int { return x * 2 })

// Get current value
let value: int = counter.get()

// Queue for producer/consumer
let queue: Queue[int] = Queue[int].new()

// Producer
spawn {
    for i in 0..100 {
        queue.push(i)
    }
    queue.close()
}

// Consumer
spawn {
    for item in queue {
        process(item)
    }
}
```

### 3.4 Async Iteration

For streaming data:

```wyn
// Async iterator
async fn fetch_pages(urls: [str]) -> async [str] {
    for url in urls {
        let response: Response = http.get(url)
        yield response.body
    }
}

// Consume async iterator
for await page in fetch_pages(urls) {
    process(page)
}

// Or collect all
let pages: [str] = await fetch_pages(urls).collect()
```

### 3.5 All I/O is Non-Blocking

Everything looks synchronous but never blocks the thread:

```wyn
// These don't block - runtime handles async internally
let response: Response = http.get("https://api.example.com")
let file: str = fs.read("data.txt")
let conn: Connection = net.connect("localhost:8080")
```

---

## 4. Module System

### 4.1 Import Syntax

```wyn
// Standard library - just the name
import http
import fs
import json

// Local files - relative path
import ./utils
import ./models/user
import ../shared/config

// External packages - URL-style
import github.com/user/package
import github.com/user/package/submodule

// Aliasing
import http as h
import github.com/user/long_package_name as pkg

// Selective imports
import { get, post } from http
import { User, Admin } from ./models/user
```

### 4.2 Module Structure

Folders are modules. Files in a folder are part of that module:

```
my-project/
├── wyn.toml
├── src/
│   ├── main.wyn           # Entry point
│   ├── utils.wyn          # import ./utils
│   └── models/
│       ├── user.wyn       # import ./models/user
│       └── product.wyn    # import ./models/product
└── tests/
    └── test_utils.wyn
```

```wyn
// src/models/user.wyn
pub struct User {
    pub name: str
    pub age: int
}

pub fn create(name: str) -> User {
    return User{name: name, age: 0}
}
```

```wyn
// src/main.wyn
import ./models/user

fn main() {
    let u: user.User = user.create("Alice")
    print(u.name)
}
```

### 4.3 Package Resolution

1. Standard library: `import http` → built-in
2. Relative path: `import ./foo` → `./foo.wyn` or `./foo/` folder
3. Package: `import github.com/x/y` → downloaded to `.wyn/deps/`

---

## 5. Standard Library

```
std/
├── io           # Streams, readers, writers
├── fs           # Filesystem operations
├── net          # TCP, UDP sockets
├── http         # HTTP client and server
├── json         # JSON encoding/decoding
├── os           # OS interaction, env, args
├── time         # Time, duration, timers
├── math         # Math functions
├── crypto       # Cryptographic primitives
├── regex        # Regular expressions
├── log          # Logging
├── test         # Test framework
├── gui          # GUI framework
└── tui          # Terminal UI framework
```

### 5.1 HTTP

```wyn
import http

// Client - GET
let response: Response = http.get("https://api.example.com/users")
let data: any = response.json()

// Client - POST with JSON
let response: Response = http.post("https://api.example.com/users", {
    "name": "Alice",
    "age": 30
})

// Server
http.serve(8080, fn(req: Request) -> Response {
    match req.path {
        "/" => return Response{html: "<h1>Hello!</h1>"}
        "/api/users" => return Response{json: get_users()}
        _ => return Response{status: 404, text: "Not found"}
    }
})
```

### 5.2 Filesystem

```wyn
import fs

// Read
let content: str = fs.read("file.txt")
let lines: [str] = fs.read("file.txt").lines()
let data: any = fs.read_json("data.json")

// Write
fs.write("file.txt", "Hello, World!")
fs.write_json("data.json", {"name": "Alice"})

// Check existence
if fs.exists("config.txt") {
    // ...
}

// List directory
for entry in fs.list(".") {
    print("{entry.name} - {entry.size} bytes")
}

// Create directory
fs.mkdir("new_folder")
fs.mkdir_all("path/to/nested/folder")
```

### 5.3 GUI Module

```wyn
import gui

fn main() {
    gui.app("My App", fn() {
        gui.window("Hello Wyn", 800, 600, fn() {
            gui.column([
                gui.text("Welcome to Wyn!"),
                gui.button("Click me", fn() { print("Clicked!") }),
                gui.input("Enter text...", fn(text: str) { print(text) })
            ])
        })
    })
}
```

### 5.4 TUI Module

```wyn
import tui

fn main() {
    tui.app(fn() {
        tui.text("Select an option:")
        tui.list(["Option 1", "Option 2", "Option 3"], fn(idx: int) {
            print("Selected: {idx}")
        })
    })
}
```

---

## 6. Testing

Tests are built into the language:

```wyn
// math.wyn
pub fn add(a: int, b: int) -> int {
    return a + b
}

pub fn multiply(a: int, b: int) -> int {
    return a * b
}

// Tests in same file or tests/ folder
test "addition works" {
    assert add(2, 3) == 5
    assert add(-1, 1) == 0
}

test "multiplication works" {
    assert multiply(3, 4) == 12
    assert multiply(0, 100) == 0
}
```

### 6.1 Assertions

```wyn
// Basic assertions
assert x == y                           // Equality
assert x != y                           // Inequality
assert x > 0                            // Comparison
assert list.len() == 3                  // Any expression

// With custom message
assert x > 0, "x must be positive"

// Check for errors
assert_err {
    parse("invalid")
}

// Approximate equality (for floats)
assert_near(0.1 + 0.2, 0.3, 0.0001)
```

### 6.2 Table-Driven Tests

```wyn
test "add with multiple inputs" {
    let cases: [(int, int, int)] = [
        (1, 2, 3),
        (0, 0, 0),
        (-1, 1, 0),
        (100, 200, 300)
    ]
    
    for (a, b, expected) in cases {
        assert add(a, b) == expected, "add({a}, {b}) should be {expected}"
    }
}
```

### 6.3 Setup/Teardown

```wyn
test "database operations" {
    let db: TestDB = create_test_db()
    defer db.destroy()
    
    db.insert({"name": "Alice"})
    assert db.count() == 1
}
```

### 6.4 Benchmarks

```wyn
bench "addition performance" {
    add(1000000, 2000000)
}
// Output: bench "addition performance": 1.2ns/op
```

### 6.5 Running Tests

```bash
wyn test              # Run all tests
wyn test math.wyn     # Run tests in specific file
wyn test -v           # Verbose output
wyn test --watch      # Re-run on file changes
wyn bench             # Run benchmarks
```

---

## 7. Operators

### 7.1 Arithmetic

```wyn
+   // Addition
-   // Subtraction
*   // Multiplication
/   // Division (integer for int, float for float)
%   // Modulo
**  // Power
```

### 7.2 Comparison

```wyn
==  // Equal
!=  // Not equal
<   // Less than
>   // Greater than
<=  // Less than or equal
>=  // Greater than or equal
```

### 7.3 Logical

```wyn
and  // Logical AND
or   // Logical OR
not  // Logical NOT
```

### 7.4 Assignment

```wyn
=    // Assign
+=   // Add and assign
-=   // Subtract and assign
*=   // Multiply and assign
/=   // Divide and assign
%=   // Modulo and assign
```

### 7.5 Ternary Operator

```wyn
let result: str = if condition then "yes" else "no"

// Nested
let grade: str = if score >= 90 then "A" 
                 else if score >= 80 then "B"
                 else if score >= 70 then "C"
                 else "F"

// With expressions
let max: int = if a > b then a else b
let abs: int = if x >= 0 then x else -x
```

### 7.6 Other

```wyn
??   // Null coalescing (default value)
?    // Error propagation
!    // Unwrap optional
?.   // Optional chaining
..   // Range (exclusive)
..=  // Range (inclusive)
...  // Spread operator
|    // Pattern alternative (in match)
```

### 7.7 Spread Operator

```wyn
// Arrays
let a: [int] = [1, 2, 3]
let b: [int] = [4, 5, 6]
let combined: [int] = [...a, ...b]  // [1, 2, 3, 4, 5, 6]

// Function arguments
fn sum(a: int, b: int, c: int) -> int {
    return a + b + c
}
let args: [int] = [1, 2, 3]
let result: int = sum(...args)

// Maps
let defaults: {str: int} = {"a": 1, "b": 2}
let overrides: {str: int} = {"b": 3, "c": 4}
let merged: {str: int} = {...defaults, ...overrides}  // {"a": 1, "b": 3, "c": 4}
```

### 7.8 List Comprehensions

Python-style comprehensions for concise transformations:

```wyn
// Basic: [expression for item in iterable]
let squares: [int] = [x * x for x in 1..10]  // [1, 4, 9, 16, 25, 36, 49, 64, 81]

// With filter: [expression for item in iterable if condition]
let evens: [int] = [x for x in 1..20 if x % 2 == 0]  // [2, 4, 6, 8, 10, 12, 14, 16, 18]

// Transform and filter
let big_squares: [int] = [x * x for x in 1..10 if x > 5]  // [36, 49, 64, 81]

// With function calls
let names: [str] = ["alice", "bob", "charlie"]
let upper: [str] = [name.upper() for name in names]

// Nested loops
let pairs: [(int, int)] = [(x, y) for x in 1..4 for y in 1..4 if x != y]

// Map comprehensions
let scores: {str: int} = {name: name.len() for name in names}  // {"alice": 5, "bob": 3, "charlie": 7}

// With condition
let passing: {str: int} = {name: score for (name, score) in results if score >= 60}
```

Comprehensions compile to the same code as explicit loops - no performance penalty.

---

## 8. Strings

All strings in Wyn are UTF-8 encoded and immutable.

### 8.1 String Literals

```wyn
// Regular strings
let s: str = "Hello, World!"

// Escape sequences
let escaped: str = "Line1\nLine2\tTabbed"
let brace: str = "Use {{x}} for literal braces"

// Raw strings (no escapes)
let raw: str = r"C:\Users\name\file.txt"
let regex: str = r"\d+\.\d+"

// Multi-line strings
let multi: str = """
    This is a multi-line string.
    Indentation is preserved.
    No need for \n escapes.
"""
```

### 8.2 String Interpolation

Use `{expression}` inside strings:

```wyn
let name: str = "Alice"
let age: int = 30

print("Hello, {name}!")                    // Hello, Alice!
print("{name} is {age} years old")         // Alice is 30 years old
print("Next year: {age + 1}")              // Next year: 31
print("Uppercase: {name.upper()}")         // Uppercase: ALICE

// Formatting
let pi: float = 3.14159
print("Pi: {pi:.2}")                       // Pi: 3.14
print("Padded: {age:05}")                  // Padded: 00030
print("Hex: {255:x}")                      // Hex: ff
```

---

## 9. Keywords

```
fn       struct   enum     interface impl     let      mut      pub
if       else     then     match     for      while    in
break    continue return   import    from     as
true     false    try      catch     defer    Self
spawn    parallel async    await     yield
some     none     
and      or       not
test     bench    assert
```

---

## 10. Compilation Targets

| Target | Status |
|--------|--------|
| x86_64-linux | Primary |
| x86_64-macos | Primary |
| x86_64-windows | Primary |
| aarch64-macos | Primary |
| aarch64-linux | Primary |
| wasm | Primary |
| aarch64-ios | Secondary |
| aarch64-android | Secondary |

### 10.1 Build Commands

```bash
wyn build main.wyn                    # Debug build
wyn build main.wyn --release          # Optimized build
wyn build main.wyn --target wasm      # WebAssembly
wyn build main.wyn --target linux     # Cross-compile
wyn build main.wyn -o myapp           # Custom output name
```

### 10.2 Self-Hosting Compiler

```
Source (.wyn) → Lexer → Parser → Type Check → IR → Code Gen → Binary
```

Bootstrap stages:
1. **Stage 0**: Minimal compiler in C
2. **Stage 1**: Wyn compiler compiled by Stage 0
3. **Stage 2**: Wyn compiler compiled by Stage 1 (ship this)

---

## 11. CLI Reference

```bash
# Running
wyn run <file>           # Compile and execute
wyn repl                 # Interactive REPL

# Building
wyn build <file>         # Compile to executable
wyn build --release      # Optimized build
wyn build --target X     # Cross-compile

# Code Quality
wyn check <file|dir>     # Type check without building
wyn fmt <file|dir>       # Format code
wyn fmt --check          # Check formatting (CI)
wyn lint <file|dir>      # Lint for issues
wyn lint --fix           # Auto-fix issues

# Testing
wyn test                 # Run all tests
wyn test <file>          # Run specific tests
wyn test -v              # Verbose
wyn test --watch         # Watch mode
wyn bench                # Run benchmarks

# Project Management
wyn new <name>           # Create new project
wyn init                 # Initialize in current dir
wyn add <package>        # Add dependency
wyn remove <package>     # Remove dependency
wyn update               # Update dependencies
wyn clean                # Remove build artifacts

# Documentation
wyn doc                  # Generate docs
wyn doc --serve          # Serve docs locally

# Info
wyn version              # Show version
wyn help                 # Show help
wyn env                  # Show environment
```

---

## 12. Example Programs

### Hello World

```wyn
fn main() {
    print("Hello, World!")
}
```

### HTTP Server

```wyn
import http

fn main() {
    print("Server running on :8080")
    http.serve(8080, fn(req: Request) -> Response {
        match req.path {
            "/" => return Response{html: "<h1>Welcome!</h1>"}
            "/api/users" => return Response{json: [{"name": "Alice"}, {"name": "Bob"}]}
            _ => return Response{status: 404}
        }
    })
}
```

### CLI Tool

```wyn
import os
import fs

fn main() {
    let args: [str] = os.args()
    
    if args.len() < 2 {
        print("Usage: wc <file>")
        return
    }
    
    let content: str = fs.read(args[1])
    let lines: int = content.lines().len()
    print("{args[1]}: {lines} lines")
}
```

### Parallel Processing

```wyn
import http

fn main() {
    let urls: [str] = [
        "https://example.com",
        "https://example.org",
        "https://example.net"
    ]
    
    let results: [(str, int)] = parallel urls.map(fn(url: str) -> (str, int) {
        let resp: Response = http.get(url)
        return (url, resp.body.len())
    })
    
    for (url, size) in results {
        print("{url}: {size} bytes")
    }
}
```

### Generic Data Structure

```wyn
pub struct Queue[T] {
    items: [T]
    
    pub fn new() -> Queue[T] {
        return Queue[T]{items: []}
    }
    
    pub fn push(mut self, item: T) {
        self.items.push(item)
    }
    
    pub fn pop(mut self) -> ?T {
        if self.items.len() == 0 {
            return none
        }
        return some(self.items.remove(0))
    }
    
    pub fn len(self) -> int {
        return self.items.len()
    }
}

fn main() {
    let q: Queue[str] = Queue[str].new()
    q.push("first")
    q.push("second")
    
    print(q.pop() ?? "empty")  // first
    print(q.pop() ?? "empty")  // second
    print(q.pop() ?? "empty")  // empty
}
```

---

## 13. GPU Computing

Wyn provides first-class GPU support for parallel computation, making it ideal for AI/ML workloads, graphics, and high-performance computing. Write GPU code in the same language as CPU code — no CUDA, no shaders, no separate toolchain.

### 13.1 GPU Blocks

Use `gpu {}` blocks to run code on the GPU:

```wyn
import gpu

fn main() {
    let data: [float] = [1.0, 2.0, 3.0, 4.0, 5.0]
    
    // Run on GPU - parallel for is automatically parallelized across GPU cores
    let result: [float] = gpu {
        parallel for i in 0..data.len() {
            data[i] = data[i] * 2.0
        }
        return data
    }
    
    print(result)  // [2.0, 4.0, 6.0, 8.0, 10.0]
}
```

### 13.2 GPU Functions

Mark functions with `@gpu` to compile them for GPU execution:

```wyn
@gpu
fn vector_add(a: [float], b: [float]) -> [float] {
    let result: [float] = [0.0; a.len()]
    parallel for i in 0..a.len() {
        result[i] = a[i] + b[i]
    }
    return result
}

@gpu
fn matrix_multiply(a: [[float]], b: [[float]]) -> [[float]] {
    let rows: int = a.len()
    let cols: int = b[0].len()
    let inner: int = b.len()
    let result: [[float]] = [[0.0; cols]; rows]
    
    parallel for i in 0..rows {
        parallel for j in 0..cols {
            mut sum: float = 0.0
            for k in 0..inner {
                sum = sum + a[i][k] * b[k][j]
            }
            result[i][j] = sum
        }
    }
    return result
}
```

### 13.3 Tensor Type

The `Tensor` type provides N-dimensional arrays optimized for GPU operations:

```wyn
import gpu

fn main() {
    // Create tensors (allocated on GPU by default)
    let a: Tensor[float] = gpu.zeros(1024, 1024)
    let b: Tensor[float] = gpu.rand(1024, 1024)
    let c: Tensor[float] = gpu.ones(1024, 1024)
    
    // Matrix operations (executed on GPU)
    let d: Tensor[float] = a @ b           // Matrix multiply
    let e: Tensor[float] = b + c           // Element-wise add
    let f: Tensor[float] = b * 2.0         // Scalar multiply
    let g: Tensor[float] = b.transpose()   // Transpose
    
    // Reductions
    let sum: float = b.sum()
    let mean: float = b.mean()
    let max_val: float = b.max()
    
    // Reshaping
    let flat: Tensor[float] = b.flatten()
    let reshaped: Tensor[float] = flat.reshape(512, 2048)
    
    // Move to CPU when needed
    let cpu_data: [[float]] = d.to_cpu()
}
```

### 13.4 Neural Network Primitives

Built-in operations for deep learning:

```wyn
import gpu
import gpu.nn

fn main() {
    let input: Tensor[float] = gpu.rand(32, 784)    // batch of 32, 784 features
    let weights: Tensor[float] = gpu.rand(784, 256)
    let bias: Tensor[float] = gpu.zeros(256)
    
    // Forward pass
    let z: Tensor[float] = input @ weights + bias
    let activated: Tensor[float] = nn.relu(z)
    let output: Tensor[float] = nn.softmax(activated)
    
    // Common layers
    let conv_out: Tensor[float] = nn.conv2d(input, kernel, stride: 1, padding: 1)
    let pooled: Tensor[float] = nn.max_pool2d(conv_out, kernel_size: 2)
    let normed: Tensor[float] = nn.batch_norm(pooled)
    let dropped: Tensor[float] = nn.dropout(normed, rate: 0.5)
}
```

### 13.5 Automatic Differentiation

Use `@autodiff` for automatic gradient computation:

```wyn
import gpu

@autodiff
fn loss(weights: Tensor[float], x: Tensor[float], y: Tensor[float]) -> float {
    let pred: Tensor[float] = x @ weights
    let diff: Tensor[float] = pred - y
    return (diff * diff).mean()
}

fn main() {
    let weights: Tensor[float] = gpu.rand(784, 10)
    let x: Tensor[float] = gpu.rand(32, 784)
    let y: Tensor[float] = gpu.rand(32, 10)
    
    // Compute loss and gradients in one call
    let (loss_val, grads) = loss.grad(weights, x, y)
    
    // Update weights
    let lr: float = 0.01
    weights = weights - grads * lr
}
```

### 13.6 Device Management

Control GPU device selection and memory:

```wyn
import gpu

fn main() {
    // List available devices
    let devices: [gpu.Device] = gpu.list_devices()
    for d in devices {
        print("{d.name}: {d.memory_gb} GB")
    }
    
    // Select device
    gpu.set_device(0)
    
    // Check memory
    let free: int = gpu.memory_free()
    let total: int = gpu.memory_total()
    
    // Explicit device placement
    let a: Tensor[float] = gpu.zeros(1000, 1000, device: 0)
    let b: Tensor[float] = gpu.zeros(1000, 1000, device: 1)
    
    // Move between devices
    let b_on_0: Tensor[float] = b.to_device(0)
    let c: Tensor[float] = a @ b_on_0
    
    // Synchronize (wait for GPU operations to complete)
    gpu.sync()
}
```

### 13.7 Mixed Precision

Support for half-precision and mixed-precision training:

```wyn
import gpu

fn main() {
    // Half precision (float16)
    let a: Tensor[float16] = gpu.rand_f16(1024, 1024)
    let b: Tensor[float16] = gpu.rand_f16(1024, 1024)
    let c: Tensor[float16] = a @ b
    
    // BFloat16 for training
    let weights: Tensor[bfloat16] = gpu.rand_bf16(784, 256)
    
    // Mixed precision - accumulate in float32
    let result: Tensor[float] = gpu.matmul_mixed(a, b)  // inputs f16, output f32
    
    // Automatic mixed precision
    @amp  // automatic mixed precision
    fn train_step(model: Model, x: Tensor[float], y: Tensor[float]) -> float {
        // Compiler automatically uses f16 where safe, f32 where needed
        let pred: Tensor[float] = model.forward(x)
        return loss(pred, y)
    }
}
```

### 13.8 GPU Compilation Targets

| Backend | Platform | Status |
|---------|----------|--------|
| Metal | macOS, iOS | Primary |
| Vulkan | Linux, Windows, Android | Primary |
| CUDA | NVIDIA GPUs | Secondary |
| WebGPU | Browser (WASM) | Secondary |

The compiler automatically selects the best backend for the target platform. Code is portable across all backends.

```bash
wyn build --target macos      # Uses Metal
wyn build --target linux      # Uses Vulkan
wyn build --target wasm       # Uses WebGPU
wyn build --gpu-backend cuda  # Force CUDA (if available)
```

### 13.9 Example: Image Processing

```wyn
import gpu
import gpu.image

fn main() {
    let img: Tensor[float] = image.load("photo.jpg")  // HxWx3 tensor
    
    // Apply filters on GPU
    let blurred: Tensor[float] = gpu {
        let kernel: Tensor[float] = image.gaussian_kernel(5, sigma: 1.0)
        return image.convolve(img, kernel)
    }
    
    // Batch process multiple images
    let images: [Tensor[float]] = load_images("./photos/")
    let processed: [Tensor[float]] = gpu {
        parallel for i in 0..images.len() {
            images[i] = image.resize(images[i], 224, 224)
            images[i] = image.normalize(images[i])
        }
        return images
    }
    
    image.save(blurred, "blurred.jpg")
}
```

### 13.10 Example: Simple Neural Network

```wyn
import gpu
import gpu.nn

struct MLP {
    w1: Tensor[float]
    b1: Tensor[float]
    w2: Tensor[float]
    b2: Tensor[float]
    
    fn new(input_size: int, hidden_size: int, output_size: int) -> MLP {
        return MLP{
            w1: gpu.rand(input_size, hidden_size) * 0.01,
            b1: gpu.zeros(hidden_size),
            w2: gpu.rand(hidden_size, output_size) * 0.01,
            b2: gpu.zeros(output_size)
        }
    }
    
    @gpu
    fn forward(self, x: Tensor[float]) -> Tensor[float] {
        let h: Tensor[float] = nn.relu(x @ self.w1 + self.b1)
        return nn.softmax(h @ self.w2 + self.b2)
    }
}

fn main() {
    let model: MLP = MLP.new(784, 256, 10)
    let x: Tensor[float] = gpu.rand(32, 784)  // batch of 32 images
    
    let predictions: Tensor[float] = model.forward(x)
    print("Output shape: {predictions.shape()}")  // [32, 10]
}
```

---

## 14. Comparison

| Feature | Wyn | Python | Go | Rust | CUDA |
|---------|-----|--------|-----|------|------|
| Syntax | Simple | Simple | Simple | Complex | Complex |
| Types | Static | Dynamic | Static | Static | Static |
| Speed | Fast | Slow | Fast | Fast | Fast |
| GPU Support | Native | PyTorch/TF | None | External | Native |
| GPU Syntax | Same as CPU | Different | N/A | Different | Different |
| Autodiff | Built-in | Library | None | Library | Manual |
| Portability | All GPUs | Framework | N/A | Backend | NVIDIA only |

---

*This specification is a living document. Version 0.2.0-draft.*
