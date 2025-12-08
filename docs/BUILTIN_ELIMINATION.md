# Builtin Elimination Progress

## Goal
Reduce from 8 builtins to 1 builtin (print only).

## Completed Changes

### substring() → String Slice Syntax [start:end]

**Date:** 2025-12-08

**Status:** ✅ Complete

**Changes Made:**

Replaced all `substring(source, start, end)` calls with slice syntax `source[start:end]` in Stage 1 compiler (`src/stage1/compiler.wyn`):

1. **has_keyword()** - Line 57
   - Before: `substring(source, pos, pos + klen)`
   - After: `source[pos:pos + klen]`

2. **count_functions()** - Lines 71, 73
   - Before: `substring(source, pos, pos + 2)` and `substring(source, pos + 2, pos + 3)`
   - After: `source[pos:pos + 2]` and `source[pos + 2:pos + 3]`

3. **count_strings()** - Lines 288, 292
   - Before: `substring(source, pos, pos + 1)` (2 occurrences)
   - After: `source[pos:pos + 1]` (2 occurrences)

4. **gen_strings()** - Lines 309, 314, 318
   - Before: `substring(source, pos, pos + 1)` (2 occurrences) and `substring(source, str_start, pos)`
   - After: `source[pos:pos + 1]` (2 occurrences) and `source[str_start:pos]`

**Total Replacements:** 8 occurrences across 4 functions

**Testing:**
- ✅ Stage 1 compiles successfully with stage0
- ✅ Stage 1 runs and generates working output
- ✅ Generated program executes correctly

**Result:** `substring()` is no longer used in Stage 1 compiler and can be removed as a builtin once Stage 1 is self-hosting.

### ord() Status

**Status:** ✅ Not Used

The `ord()` builtin is not used anywhere in the Stage 1 compiler, so no changes were needed.

## Remaining Builtins to Eliminate

Once Stage 1 is self-hosting, these can be moved to stdlib:

1. ✅ `substring()` - Replaced with slice syntax `[start:end]`
2. ✅ `ord()` - Not used in Stage 1
3. ⏳ `assert()` - Move to `import test`
4. ⏳ `exit()` - Move to `import os`
5. ⏳ `args()` - Move to `import os`
6. ⏳ `int_to_str()` - Move to `import string`
7. ⏳ `system()` - Move to `import os`
8. ⏳ `write_file()` - Move to `import io`

**Target:** Only `print()` remains as a builtin.
