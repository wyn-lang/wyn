# Builtin Elimination Progress

## Goal
Reduce from 8 builtins to 1 builtin (print only).

## Current Status: 4 Builtins

**Reduced from 6 to 4 builtins!**

### Current Builtins:
1. `print()` - **TARGET TO KEEP**
2. `assert()` - Cannot move (conflicts with `test` keyword)
3. `args()` - Compiler intrinsic (generates inline code)
4. `syscall()` - Compiler intrinsic (needed for stdlib)

### Eliminated Builtins:
1. ✅ `ord()` - Use `string.ord()` instead
2. ✅ `substring()` - Use slice syntax `[start:end]` or `string.substring()`

## Completed Changes

### ord() → string.ord()

**Date:** 2025-12-09

**Status:** ✅ Complete

**Changes Made:**
- Updated Stage 1 compiler to use `string.ord()` instead of bare `ord()`
- Removed `ord` from builtin list in stage0.c
- `string.ord()` still works via module mapping

### substring() → Slice Syntax

**Date:** 2025-12-08

**Status:** ✅ Complete

**Changes Made:**
- Replaced all `substring(source, start, end)` calls with slice syntax `source[start:end]` in Stage 1 compiler
- Removed `substring` from builtin list in stage0.c
- `string.substring()` still works via module mapping

## Why Some Builtins Must Stay

### args()
The `args()` function generates inline assembly to access `argc` and `argv` from the program entry point. It cannot be implemented as a library function because it needs access to the stack frame at program start.

### syscall()
The `syscall()` function is a compiler intrinsic that generates the actual system call instruction. It's the foundation for all stdlib I/O, OS, and time functions.

### assert()
The `assert()` function could theoretically move to `test.assert()`, but `test` is a reserved keyword for test blocks, causing parser conflicts.

## Path to 1 Builtin

To reach the goal of only `print()` as a builtin:

1. **assert()** - Could be renamed to something like `check()` or moved to a different module name
2. **args()** - Could become a compiler intrinsic that's not exposed as a builtin
3. **syscall()** - Could become a compiler intrinsic that's not exposed as a builtin

The key insight is that `args()` and `syscall()` are really compiler intrinsics, not builtins. They generate inline code rather than calling a function.

## Testing

All changes verified:
- ✅ Stage 1 compiles successfully
- ✅ Stage 1 runs and generates working output
- ✅ Core tests pass (arithmetic, control_flow, function, builtins)
- ✅ `string.ord()` works
- ✅ `string.substring()` works
- ✅ Slice syntax `[start:end]` works
