# Wyn Design Principles

## Core Philosophy

**Simple > Everything else**

Wyn prioritizes simplicity and consistency above all other concerns.

## The One Way Principle

**"One obvious way to do things"**

When there are multiple ways to accomplish something, Wyn chooses one and makes it the standard. This reduces cognitive load and makes code more predictable.

### Examples

❌ **Bad - Multiple ways:**
```wyn
print("hello")          // top-level function
io.print("hello")       // module function
"hello".print()         // method
```

✅ **Good - One way:**
```wyn
io.print("hello")       // Always use module function for I/O
```

## API Organization

### Methods vs Module Functions

**Use methods** when the operation is a property or transformation of the object:
```wyn
"hello".len()           // Property of the string
"hello".contains("l")   // Query about the string
[1,2,3].len()          // Property of the array
x.abs()                // Transformation of the number
```

**Use module functions** when the operation is an action involving the object:
```wyn
io.print("hello")       // Action: print to stdout
fs.read_file("x")       // Action: read from filesystem
math.sin(1.0)          // Action: calculate sine
```

**Rule of thumb:** If you can say "the string's length" or "the string contains X", use a method. If you say "print the string" or "read the file", use a module function.

## No Top-Level Pollution

All functionality lives in modules or as methods. The only top-level constructs are:
- Language keywords: `fn`, `let`, `if`, `while`, `for`, `match`, `return`, etc.
- Import statements: `import std/io`
- Type definitions: `struct`, `enum`, `interface`

No top-level functions like `print()`, `len()`, `abs()`.

## Explicit Over Implicit

```wyn
// Explicit type annotations
let x: int = 42
let y: float = 3.14

// Explicit imports
import std/io
io.print("hello")

// Explicit error handling
let result: Result[str, str] = fs.read_file("x")
let content: str = result?  // Explicit propagation
```

## Consistency

### Naming Conventions
- **Modules**: lowercase with underscores: `std/io`, `std/file_system`
- **Functions**: snake_case: `read_file`, `to_upper`
- **Types**: PascalCase: `User`, `HttpRequest`
- **Constants**: SCREAMING_SNAKE_CASE: `MAX_SIZE`, `DEFAULT_PORT`
- **Variables**: snake_case: `user_name`, `total_count`

### Method Naming
- **Query methods**: `contains()`, `is_empty()`, `has_value()`
- **Transformation methods**: `to_upper()`, `to_int()`, `reverse()`
- **Property methods**: `len()`, `size()`, `count()`

## Simplicity in Practice

### Keep It Minimal
- No operator overloading (except built-in types)
- No implicit conversions
- No multiple inheritance
- No macros or metaprogramming (Stage 0)
- No reflection (Stage 0)

### One Feature, One Purpose
- Arrays for ordered collections
- Maps for key-value pairs
- Structs for data grouping
- Enums for variants
- Interfaces for polymorphism

Don't try to make one feature do everything.

## Error Handling

**Explicit, not exceptions:**
```wyn
// Use Result type
fn read_config() -> Result[Config, str] {
    let content: str = fs.read_file("config.txt")?
    return ok(parse(content))
}

// Use Optional type
fn find_user(id: int) -> ?User {
    if id < 0 { return none }
    return some(users[id])
}
```

No hidden control flow with try/catch.

## Performance

**Fast by default, explicit when not:**
```wyn
// Default: stack allocated, fast
let arr: [int] = [1, 2, 3]

// Explicit: heap allocated when needed
let big: [int] = array.new(1000000)

// Explicit: manual memory management when needed (Stage 1)
@manual
let ptr: *int = malloc(size)
```

## Concurrency

**Explicit parallelism:**
```wyn
// Explicit spawn
spawn {
    do_work()
}

// Explicit parallel loop
parallel for item in items {
    process(item)
}
```

No hidden threads or async magic.

## Summary

1. **One obvious way** - No multiple patterns for the same thing
2. **Modules, not globals** - Everything organized, nothing top-level
3. **Methods for properties** - Functions for actions
4. **Explicit over implicit** - No surprises
5. **Simple over clever** - Readable code wins

These principles guide all language design decisions.
