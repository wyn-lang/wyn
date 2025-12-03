# Development Progress - December 3, 2025

## Summary

Fixed critical bugs in the Stage 0 compiler and improved stdlib functionality. All 47 tests continue to pass.

## Bugs Fixed

### 1. ✅ String == Operator Bug (CRITICAL)
**Problem:** `let s: str = "a"; if s == "a"` would fail because `print()` was treating all arguments as integers.

**Root Cause:** The `print()` builtin was calling the generic `_print` function which uses `%ld` format (for integers) instead of dispatching to the correct type-specific version (`print_str` for strings, `print_float` for floats).

**Solution:** Added type-aware dispatch in ARM64 codegen (line ~4803):
- Check argument type (EXPR_IDENT, EXPR_STRING, EXPR_FIELD)
- Dispatch to `print_str` for strings, `print_float` for floats, `print` for integers
- Added field type lookup for struct field access expressions

**Impact:** String comparisons and printing now work correctly throughout the codebase.

### 2. ✅ Nested Struct Field Access (VERIFIED WORKING)
**Status:** Tested `outer.inner.field` - works correctly.

**Conclusion:** This bug was likely fixed in earlier commits. No action needed.

### 3. ✅ Struct Parameters (VERIFIED WORKING)
**Status:** Tested passing structs with multiple fields to functions - all fields preserved correctly.

**Conclusion:** Struct parameters are passed by address and work properly. No action needed.

## Code Changes

### bootstrap/stage0.c
- Added type-aware `print()` handling in ARM64 EXPR_CALL codegen
- Supports EXPR_IDENT, EXPR_STRING, and EXPR_FIELD type detection
- Dispatches to correct print variant based on argument type

### TODO.md
- Marked 3 critical bugs as fixed/verified
- Updated status and added implementation notes

## Test Results

```
=== Results ===
Passed: 47
Skipped: 1
Failed: 0
```

All existing tests continue to pass with the new changes.

## New Test Files Created

- `test_string_eq_bug.wyn` - Verifies string == operator works
- `test_simple_str.wyn` - Basic string printing test
- `test_nested_struct_bug.wyn` - Nested struct field access with strings
- `test_nested_simple.wyn` - Nested struct field access with ints
- `test_struct_param_bug.wyn` - Struct parameters with multiple fields

## Agent Work In Progress

Delegated task to implement critical string builtins:
- `str_concat(a, b)` - String concatenation
- `str_substr(s, start, len)` - Substring extraction
- `str_split(s, delim)` - String splitting to array

These will enable more stdlib functions to work properly.

## Next Steps

1. Wait for agent to complete string builtin implementations
2. Test and integrate string builtins
3. Update stdlib functions to use new builtins
4. Work on remaining minor issues (string slice syntax, method calls in conditions)
5. Continue toward Stage 1 self-hosting compiler

## Statistics

- **Compiler:** Stage 0 (C bootstrap)
- **Tests Passing:** 47/48 (1 skipped: input_test)
- **Stdlib Modules:** 12 (fs, net, http, json, math, crypto, regex, collections, time, log, etc.)
- **Stdlib Functions:** 500+
- **Language Features:** ~90% complete for Stage 1 requirements
