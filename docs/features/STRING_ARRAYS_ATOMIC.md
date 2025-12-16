# String Arrays and Atomic Operations Implementation

## Status Summary

### ✅ Implemented

1. **str_split() Runtime Function**
   - Properly splits strings into Wyn arrays
   - Returns `int64_t*` (Wyn array format: [length, capacity, elem0, elem1, ...])
   - Handles any delimiter
   - Memory allocated correctly

2. **Atomic Operations Runtime**
   - `wyn_atomic_add(ptr, value)` - Atomic add, returns old value
   - `wyn_atomic_sub(ptr, value)` - Atomic subtract, returns old value
   - `wyn_atomic_load(ptr)` - Atomic load
   - `wyn_atomic_store(ptr, value)` - Atomic store
   - `wyn_atomic_cas(ptr, expected, desired)` - Compare-and-swap
   - Uses C11 `<stdatomic.h>` for real atomic operations

3. **Module Interfaces**
   - `std/string.wyn` - Added `split(s, delim)` function
   - `std/atomic.wyn` - Complete atomic operations module
   - Module mappings added to compiler

4. **Type System Updates**
   - `get_builtin_return_type()` recognizes str_split returns `[str]`
   - LLVM declarations added for all functions

### ⚠️ Partially Working

**String Arrays**:
- ✅ Can create: `const arr: [str] = string.split("a,b,c", ",")`
- ✅ Can store in variables
- ❌ Cannot index: `arr[0]` - type mismatch (i64 vs i8*)
- ❌ Cannot iterate: `for s in arr` - type mismatch
- ❌ Cannot compare: `arr[0] == "test"` - type mismatch

**Root Cause**: Array elements are stored as `i64` but strings are `i8*`. Need casting infrastructure.

**Atomic Operations**:
- ✅ Functions implemented and callable
- ❌ Require `&` (address-of) operator to work properly
- ❌ Address-of operator exists but may not work with all variable types

### ❌ Still Needed

#### 1. String Array Element Access

**Problem**: When indexing `arr[i]` where arr is `[str]`, we get `i64` but need `i8*`.

**Solution**: Add type-aware indexing in code generator:
```c
// In llvm_expr() case EXPR_INDEX:
if (array_type->inner->kind == TYPE_STR) {
    // Load i64, cast to i8*
    int elem_i64 = llvm_new_temp(lg);
    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", elem_i64, elem_ptr);
    int elem_str = llvm_new_temp(lg);
    llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", elem_str, elem_i64);
    *result_reg = elem_str;
}
```

#### 2. String Array Iteration

**Problem**: For loops over `[str]` arrays need to cast elements.

**Solution**: Update for loop codegen to handle string arrays:
```c
// In llvm_stmt() case STMT_FOR:
if (iter_type->kind == TYPE_ARRAY && iter_type->inner->kind == TYPE_STR) {
    // Cast element from i64 to i8*
    int elem_i64 = llvm_new_temp(lg);
    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", elem_i64, elem_ptr);
    int elem_str = llvm_new_temp(lg);
    llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", elem_str, elem_i64);
    // Use elem_str as loop variable
}
```

#### 3. Address-Of Operator for Atomics

**Problem**: `&variable` operator exists but needs testing/fixes.

**Solution**: Verify EXPR_ADDR works correctly in all backends.

### Code Changes Made

**Runtime** (`runtime/builtins.c`):
- Lines 168-206: Implemented real `str_split()` function
- Lines 1528-1550: Added atomic operations

**Compiler** (`bootstrap/stage0.c`):
- Line 1193-1200: Added str_split to return type mapping
- Lines 1085-1090: Added atomic module mapping
- Line 11606: Added `is_array_builtin` flag
- Line 11663: Handle array-returning builtins
- Line 11418: Handle array returns from module.function()
- Line 12727: Updated str_split LLVM declaration
- Lines 12848-12853: Added atomic LLVM declarations

**Standard Library**:
- `std/string.wyn`: Added `split()` function
- `std/atomic.wyn`: New module with all atomic operations

### Testing

**What Works**:
```wyn
// String split compiles
const arr: [str] = string.split("a,b,c", ",")

// Atomic operations compile
import atomic
// atomic.add(&counter, 1)  // Needs & operator
```

**What Doesn't Work Yet**:
```wyn
// Indexing string arrays
const first: str = arr[0]  // Type error

// Iterating string arrays
for s in arr {  // Type error
    print(s)
}
```

### Estimated Effort to Complete

1. **String Array Indexing**: 2-3 hours
   - Add type-aware casting in EXPR_INDEX
   - Handle in all backends (LLVM, x86, ARM)

2. **String Array Iteration**: 1-2 hours
   - Update for loop codegen
   - Cast elements during iteration

3. **Address-Of Operator**: 1-2 hours
   - Test and fix EXPR_ADDR
   - Ensure works with atomic operations

**Total**: 4-7 hours to complete all three features

### Next Steps

1. Add inttoptr/ptrtoint casts for string array elements
2. Update for loop to handle string arrays
3. Test & operator with atomic operations
4. Create comprehensive tests
5. Update parallel test runner to use Wyn

### Files Modified

- `runtime/builtins.c` - str_split and atomic operations
- `bootstrap/stage0.c` - Type system and codegen updates
- `std/string.wyn` - Added split function
- `std/atomic.wyn` - New atomic module
- `tests/string_array_test.wyn` - Test file (not yet passing)
- `tests/atomic_simple_test.wyn` - Basic test

### Conclusion

✅ **Foundation is complete** - Runtime functions work, modules exist
⚠️ **Type casting needed** - String arrays need i64 ↔ i8* conversion
🎯 **4-7 hours to full functionality** - Straightforward codegen work
