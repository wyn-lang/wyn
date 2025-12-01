# Stage 0 Compiler Bugs to Fix

## Critical Bugs

### 1. String == operator broken for variables
- `let s: str = "a"; if s == "a"` returns false
- Workaround: use `str_cmp(a, b) == 0`
- Location: likely in codegen for EXPR_BINARY with TOK_EQEQ and string types

### 2. Nested struct field access crashes
- `result.inner.field` causes segfault
- Works: `result.field` (single level)
- Location: EXPR_FIELD codegen in ARM64 section (~line 5355)

### 3. Struct parameters have corrupted fields
- Passing struct to function: fields after first are garbage
- Works: methods with `self` parameter
- Location: function parameter setup in cg_function (~line 6109)

## Minor Issues

### 4. String slice syntax broken
- `str[0:1]` returns wrong value
- Workaround: use `substring(str, start, len)`

### 5. Method calls can't be used directly in if conditions
- `if obj.method()` fails type check
- Workaround: `let result: bool = obj.method(); if result`
