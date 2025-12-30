# Array Method Syntax Issue - Technical Analysis

## Problem

`s = s.append(item)` doesn't work due to LLVM type mismatch.

## Root Cause

Arrays in Wyn have a type system mismatch:
- **Type system**: Arrays are `TYPE_ARRAY` (value type)
- **LLVM IR**: Arrays are `i64*` (pointer type)
- **Return type**: Extension methods return `i64` but array functions return `i64*`

## Error

```
error: '%4' defined with type 'ptr' but expected 'i64'
  ret i64 %4
```

The function signature says `-> array` (which maps to `i64` in LLVM) but `array.append()` returns `i64*`.

## Why .len() Works But .append() Doesn't

**.len() returns int:**
```llvm
%result = call i64 @str_len(i8* %obj)  // Returns i64 ✅
ret i64 %result                         // Matches signature ✅
```

**.append() returns array:**
```llvm
%result = call i64* @array_append(i64* %arr, i64 %item)  // Returns i64* ✅
ret i64 %result                                           // Type mismatch ❌
```

## Solutions

### Option 1: Fix Type System (Proper Fix)
Make arrays consistently pointers in both type system and LLVM:
- Change `TYPE_ARRAY` representation
- Update all array operations
- **Effort**: 1-2 weeks
- **Risk**: High (breaks existing code)

### Option 2: Add Pointer Casting (Hack)
Cast `i64*` to `i64` and back:
```llvm
%ptr = call i64* @array_append(...)
%int = ptrtoint i64* %ptr to i64
ret i64 %int
```
- **Effort**: 2-3 days
- **Risk**: Medium (ugly but works)

### Option 3: Built-in Method (Workaround)
Make `.append()` a compiler built-in like `.len()`:
- Handle in codegen directly
- No extension method needed
- **Effort**: 1 day
- **Risk**: Low

### Option 4: Keep Current Syntax (Pragmatic)
Document that `s = array.append(s, item)` is the correct syntax:
- No changes needed
- Functional programming style
- **Effort**: 0 days
- **Risk**: None

## Recommendation

**Option 3 (Built-in Method)** for immediate fix, then **Option 1 (Fix Type System)** for v0.5.0.

This gives users `s = s.append(item)` quickly while planning proper fix.

## Implementation Plan (Option 3)

1. Add `.append()` to method call handling (like `.len()`)
2. Generate `array_append` call directly
3. Handle pointer/value conversion inline
4. Test thoroughly

**Estimated time**: 4-6 hours

---

**Status**: Documented, needs implementation
**Priority**: High
**Complexity**: Medium
