# Control Flow Fixes - 2025-12-11

## Summary
Fixed break/continue statement handling in LLVM backend to prevent invalid IR generation.

## Problem
When `break` or `continue` statements appeared inside `if` blocks, the compiler would generate:
1. A branch to the loop end/start label (correct)
2. An additional unconditional branch to the if-statement's end label (incorrect)

This created invalid LLVM IR with unreachable code and out-of-order register numbering.

## Solution
Added `block_ends_with_break_or_continue()` function to check if a block terminates with break/continue, similar to the existing `block_ends_with_return()` check.

Modified the if-statement LLVM codegen to skip emitting the end-label branch when the then/else blocks end with break, continue, or return.

## Files Modified
- `bootstrap/stage0.c`:
  - Added `block_ends_with_break_or_continue()` at line ~3820
  - Updated STMT_IF case in LLVM codegen at line ~11645

## Tests Fixed
✅ `examples/break_continue_test.wyn` - break/continue in loops
✅ `tests/control_flow_test.wyn` - comprehensive control flow
✅ `tests/for_loop_edge_test.wyn` - for loop edge cases
✅ `tests/arithmetic_test.wyn` - arithmetic operations (was affected by control flow)
✅ `tests/comparison_test.wyn` - comparison operations (was affected by control flow)

## Results
- **Before**: 86 passed, 88 failed (49%)
- **After**: 89 passed, 85 failed (51%)
- **Improvement**: +3 tests passing

## Example
```wyn
while i < 10 {
    if i == 5 {
        break  // Now correctly branches to loop end without extra branch
    }
    print(i)
    i += 1
}
```

## Technical Details
The fix ensures that when a basic block ends with a terminator instruction (return, break, or continue), no additional branch instruction is emitted. This maintains LLVM's SSA form and prevents unreachable code.
