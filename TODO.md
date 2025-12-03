# Stage 0 Compiler Bugs to Fix

## Critical Bugs

### 1. ✅ FIXED: String == operator broken for variables
- ~~`let s: str = "a"; if s == "a"` returns false~~
- ~~Workaround: use `str_cmp(a, b) == 0`~~
- Fixed: Added type-aware print() dispatch in ARM64 codegen (line ~4803)

### 2. ✅ VERIFIED WORKING: Nested struct field access
- ~~`result.inner.field` causes segfault~~
- Tested and working correctly in current version
- May have been fixed in earlier commits

### 3. ✅ VERIFIED WORKING: Struct parameters
- ~~Passing struct to function: fields after first are garbage~~
- Tested and working correctly in current version
- Struct parameters are passed by address and work properly

## Minor Issues

### 4. String slice syntax broken
- `str[0:1]` returns wrong value
- Workaround: use `substring(str, start, len)`

### 5. Method calls can't be used directly in if conditions
- `if obj.method()` fails type check
- Workaround: `let result: bool = obj.method(); if result`
