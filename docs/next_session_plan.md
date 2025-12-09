# Next Session Action Plan

## Goal
Get AST compiler working with while loops and variable assignments.

## Priority Tasks

### 1. Fix Array Handling (30 min)
**Problem:** ast_compiler.wyn segfaults when using arrays
**Solution:** Simplify to avoid complex array operations
- Option A: Use linked list of statements instead of arrays
- Option B: Pre-allocate fixed-size arrays
- Option C: Rewrite without arrays (like minimal.wyn)

**Test:** Compile examples/ast_test.wyn without segfault

### 2. Add While Loop Codegen (20 min)
**Location:** ast_compiler.wyn, gen_stmt() function
**Add:**
```wyn
if stmt.kind == 3 {  # while loop
    io.append_file(f, "L0:\n")
    # Generate condition check
    io.append_file(f, "    b.ge L1\n")
    # Generate body
    io.append_file(f, "    b L0\n")
    io.append_file(f, "L1:\n")
}
```

**Test:** Compile examples/stage1_simple.wyn

### 3. Add Assignment Codegen (15 min)
**Location:** ast_compiler.wyn, gen_stmt() function
**Add:**
```wyn
if stmt.kind == 4 {  # assignment
    # Evaluate right side
    # Store to variable
}
```

**Test:** `x = x + 1` works

### 4. Integrate Expression Evaluation (25 min)
**Problem:** expr_eval.wyn uses global vars and string concat
**Solution:** Rewrite to pass state as parameters
- Remove global `vars` array
- Pass VarInfo array as parameter
- Use io.append_file instead of string concat

**Test:** Variable references work in expressions

## Success Criteria

✅ ast_compiler.wyn compiles without errors
✅ Runs without segfault on simple input
✅ Generates correct assembly for:
   - print("Hello")
   - let x: int = 0
   - while x < 3 { ... }
   - x = x + 1

## Test Cases (In Order)

1. `examples/ast_test.wyn` - Just print
2. `examples/stage1_simple.wyn` - While loop with variable
3. `examples/stage1_test.wyn` - Multiple variables

## Fallback Plan

If arrays are too problematic:
1. Use minimal.wyn as base
2. Add features one at a time
3. Hard-code for specific test cases first
4. Generalize later

## Time Budget

- Array fixes: 30 min
- While loops: 20 min
- Assignments: 15 min
- Integration: 25 min
- Testing: 10 min
**Total: 100 min (1.5 hours)**

## Delegation Opportunities

While I work on array fixes:
- Delegate: Rewrite expr_eval.wyn without globals
- Delegate: Create more test cases
- Delegate: Document AST structure

## Notes

- Keep changes minimal
- Test after each change
- Commit working versions
- Don't try to be perfect - get it working first
