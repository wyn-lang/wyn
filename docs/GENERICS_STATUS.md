# Generics Implementation Status

**Last Updated:** December 16, 2025
**Status:** Partially Implemented (Parser ✅, Codegen ❌)

## What Works

### ✅ Parser
Generic syntax is fully parsed:
```wyn
fn identity[T](x: T) -> T {
    return x
}

fn main() {
    const a = identity[int](42)
}
```

**Parses correctly:**
- Generic type parameters: `[T]`, `[T, U]`
- Generic function calls: `func[int](arg)`
- Generic structs: `struct Box[T] { value: T }`

### ✅ Type Checker
- Recognizes generic types
- Stores type arguments
- Basic validation

## What Doesn't Work

### ❌ Codegen (Monomorphization)
**Problem:** Generic types aren't replaced with concrete types

**Current behavior:**
```wyn
fn identity[T](x: T) -> T {
    return x  // T is undefined in codegen
}
```

**Needed behavior:**
```wyn
// When called with identity[int](42)
// Should generate:
fn identity_int(x: int) -> int {
    return x
}
```

## Implementation Plan

### Phase 1: Monomorphization (4-6 hours)

**Step 1:** Track generic instantiations
```c
typedef struct {
    FnDef* generic_fn;
    Type** concrete_types;
    int type_count;
    FnDef* instantiated_fn;
} GenericInstance;
```

**Step 2:** Create instantiated functions
- When `identity[int]` is called
- Clone the generic function
- Replace T with int throughout
- Generate code for concrete version

**Step 3:** Type substitution
- Replace generic type T with concrete type
- In parameters, return type, body
- Recursively through all expressions

### Phase 2: Generic Structs (2-3 hours)
```wyn
struct Box[T] {
    value: T
}

const box = Box[int] { value: 42 }
```

### Phase 3: Generic Constraints (2-3 hours)
```wyn
fn add[T: Numeric](a: T, b: T) -> T {
    return a + b
}
```

## Current Workaround

Use concrete types directly:
```wyn
// Instead of:
fn identity[T](x: T) -> T { return x }

// Use:
fn identity_int(x: int) -> int { return x }
fn identity_str(x: str) -> str { return x }
```

## Why Generics Matter

### Enable Result/Option Types
```wyn
enum Result[T, E] {
    Ok(T),
    Err(E)
}

enum Option[T] {
    Some(T),
    None
}
```

### Enable Generic Collections
```wyn
struct Array[T] {
    data: T*
    length: int
}

struct HashMap[K, V] {
    // ...
}
```

### Enable Reusable Code
```wyn
fn map[T, U](arr: [T], f: fn(T) -> U) -> [U] {
    // Transform array
}
```

## Implementation Complexity

**Difficulty:** High
- Requires type substitution throughout AST
- Need to track instantiations
- Need to generate multiple versions
- Complex interactions with type checker

**Time Estimate:** 8-12 hours for full implementation

**Priority:** High (enables Result/Option, better collections)

## Alternative: Simple Generics

**Simpler approach:** Only support built-in generic types

```wyn
// Built-in generic types (hardcoded)
enum Result[T, E] { Ok(T), Err(E) }
enum Option[T] { Some(T), None }

// No user-defined generics
```

**Time:** 2-3 hours
**Benefit:** Gets us Result/Option quickly
**Drawback:** Not fully generic

## Recommendation

1. **Short term:** Implement built-in Result/Option without full generics
2. **Long term:** Complete full generic system

This gets us error handling quickly while we work on full generics.

## Next Steps

1. Add Result/Option as built-in types (2 hours)
2. Add pattern matching for Result/Option (1 hour)
3. Later: Complete full generic system (8 hours)

**Status:** Generics are 30% complete (parser done, codegen TODO)
