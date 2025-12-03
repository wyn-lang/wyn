# Stdlib Refactoring Plan

## Problem
Currently we have mixed patterns:
- Builtin methods: `"hello".len()`, `x.abs()`
- Stdlib functions: `reverse(s)`, `replace(s, old, new)`

This violates the "one obvious way" principle.

## Goal
Everything should use method syntax:
- `"hello".reverse()` instead of `reverse("hello")`
- `s.replace(old, new)` instead of `replace(s, old, new)`
- `arr.sum()` instead of `sum(arr)`

## Implementation Strategy

### Option 1: Add as Builtin Methods (Recommended)
Add common methods directly to the compiler (like `.len()`, `.abs()`):
- `str.split(delim)` -> calls str_split builtin
- `str.replace(old, new)` -> calls replace logic
- `str.contains(substr)` -> calls str_find
- `str.reverse()` -> reverse logic
- `arr.sum()`, `arr.max()`, `arr.min()` -> array operations

**Pros:**
- Fast (no function call overhead)
- Type-safe
- Consistent with existing `.len()`, `.abs()`

**Cons:**
- Bloats compiler
- Harder to extend

### Option 2: Extension Methods (Stage 1)
Wait for Stage 1 to support extension methods:
```wyn
// In std/string.wyn
fn (self: str) reverse() -> str {
    // implementation
}
```

**Pros:**
- Clean separation
- Easy to extend
- Stdlib stays in Wyn code

**Cons:**
- Requires Stage 1 feature
- Not available now

### Option 3: Hybrid (Current)
Keep both patterns:
- Builtins use methods
- Stdlib uses functions

**Pros:**
- Works now
- No refactoring needed

**Cons:**
- Violates "one way" principle
- Inconsistent API

## Recommendation

**Short term (Stage 0):** Keep current hybrid approach, document it clearly

**Long term (Stage 1):** Implement extension methods, refactor stdlib to use method syntax

## Migration Path

1. Stage 0: Document that stdlib functions will become methods in Stage 1
2. Stage 1: Add extension method syntax
3. Stage 1: Refactor stdlib to use extension methods
4. Stage 1: Deprecate top-level function versions
5. v1.0: Remove top-level functions entirely

## Examples

### Current (Stage 0):
```wyn
let s: str = "hello world"
let upper: str = to_upper(s)
let parts: [str] = split(s, " ")
let replaced: str = replace(s, "world", "Wyn")
```

### Future (Stage 1+):
```wyn
let s: str = "hello world"
let upper: str = s.to_upper()
let parts: [str] = s.split(" ")
let replaced: str = s.replace("world", "Wyn")
```

## Priority

**Low** - This is a design consistency issue, not a functionality issue. Focus on:
1. Fixing str_split bug
2. Implementing missing builtins
3. Getting to Stage 1 self-hosting
4. Then refactor for consistency
