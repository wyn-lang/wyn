# Standard Library Issues Found - 2025-11-30

## Critical Issues Fixed

### utils.wyn
- ✅ **FIXED**: Removed `divmod()` function that returned unsupported tuple type `(int, int)`
- ✅ **FIXED**: Split `spring()` into `spring_position()` and `spring_velocity()` to avoid tuple return
- ✅ **FIXED**: Fixed `snap()` function using undefined `round()` - implemented inline rounding

## Issues Requiring Language Features

### Multiple Files - Multiline String Literals
Files affected: `io.wyn`, `log.wyn`, `test.wyn`, `time.wyn`, `os.wyn`, `json.wyn`

**Issue**: Functions use multiline string literals (token 87 = `\n` in string context) which aren't supported by Stage 0 compiler.

**Example**:
```wyn
fn print_error(msg: str) {
    print_str("ERROR: " + msg + "\n")  # This works
}

fn print_multiline() {
    print_str("Line 1
Line 2")  # This doesn't work - multiline literal
}
```

**Impact**: Many utility print functions don't compile but aren't used by tests.

### log.wyn - Enum Variant Syntax
**Issue**: Uses `LogLevel::Debug` syntax which isn't supported. Stage 0 only supports simple enum values.

**Example**:
```wyn
match level {
    LogLevel::Debug => return "DEBUG"  # Not supported
}
```

**Workaround**: Use integer enum values directly or restructure.

### error.wyn - Match Pattern Variables
**Issue**: Uses match expressions with pattern variable extraction like `ok(value)` and `some(value)` which aren't fully implemented.

**Status**: ✅ **FIXED** - Simplified to basic helper functions without pattern matching.

### string.wyn - Massive Duplication
**Issue**: File contains 1826 lines with many duplicate function definitions:
- `index_of` defined 4 times (lines 171, 222, 502, 1236)
- `capitalize` defined 2 times
- `contains` defined 2 times
- `count_substr` defined 3 times
- Many more duplicates...

**Additional Issues**:
- Type mismatches: `index_of` returns `int` in some places, `?int` in others
- Undefined function calls: `to_lower_char`, `to_upper_char` used but not defined
- Match expressions with optional types that don't work

**Impact**: File doesn't compile but basic string functions work via builtins.

### Hexadecimal Literals Not Supported
Files affected: `bits.wyn`, `hash.wyn`, `crypto.wyn`, `random.wyn`

**Issue**: Code uses hex literals like `0xFF`, `0x1F` which aren't supported (token 8 error).

**Example**:
```wyn
let mask: int = 0xFF  # Not supported
let mask: int = 255   # Use decimal instead
```

### collections.wyn - Type Inference Issue
**Issue**: Line 572 has return type mismatch with generic types.

### http.wyn - Undefined Variables
**Issue**: Lines 277, 287 reference undefined variable `parts` in string manipulation.

### mobile.wyn - Missing Dependencies
**Issue**: References undefined types and functions:
- `PermissionStatus` type
- `BiometricType` type  
- `NetworkStatus` type
- Trigonometric functions: `sin`, `cos`, `atan2`

### tui.wyn - Mutability Issues
**Issue**: Attempts to assign to immutable variables `codes`, `display`, `result`.

**Fix**: Add `mut` keyword to variable declarations.

### regex.wyn, test.wyn - Syntax Errors
**Issue**: Complex syntax patterns that Stage 0 parser doesn't support.

### gui.wyn, net.wyn - Parse Errors
**Issue**: Various syntax constructs not supported by Stage 0.

## Files That Compile Successfully

✅ **array.wyn** - All array utilities compile
✅ **math.wyn** - All 114 math functions compile  
✅ **fs.wyn** - All filesystem path utilities compile
✅ **utils.wyn** - All utility functions compile (after fixes)

## Recommendations

### Short Term
1. ✅ Fix critical compilation errors in core utilities (DONE)
2. Remove or comment out functions using unsupported features
3. Add compiler feature flags to skip advanced functions

### Medium Term
1. Add multiline string literal support to compiler
2. Implement full enum variant syntax (`Type::Variant`)
3. Complete match pattern variable extraction
4. Add hexadecimal literal support

### Long Term
1. Deduplicate string.wyn (reduce from 1826 to ~600 lines)
2. Implement missing types for mobile/gui modules
3. Add trigonometric function builtins
4. Complete generic type inference

## Test Suite Status

✅ **All 37 tests pass** - Test suite doesn't use problematic library functions.

The core language features work correctly. Library issues are isolated to advanced features not yet needed by the compiler or tests.
