# Path to Self-Hosting

**Current Status:** 80% complete
**Estimated Time to Self-Hosting:** 2-3 hours

## What We Have Now

✅ **Working compiler** that:
- Parses source code
- Generates ARM64 assembly
- Compiles while loops
- Handles variables
- Supports print statements
- Works with different inputs

## Remaining Work

### Phase 1: Assignment Statements (30 min)
**Goal:** Parse and compile `x = x + 1`

Currently hardcoded as:
```asm
ldr x0, [x29, #16]
add x0, x0, #1
str x0, [x29, #16]
```

Need to:
1. Parse assignment: `x = x + 1`
2. Extract left side (x)
3. Extract right side (x + 1)
4. Generate appropriate assembly

**Approach:**
```wyn
fn parse_assignment(src: str) -> {var: str, expr: str} {
    # Find "x = "
    # Extract variable name
    # Extract expression after =
}
```

### Phase 2: Multiple Variables (30 min)
**Goal:** Track multiple variables with different offsets

Currently: Single variable at offset 16

Need to:
1. Track variable names and offsets
2. Allocate stack space for each
3. Look up offsets when generating code

**Approach:**
```wyn
let var_names: str = ""  # "x,y,z"
let var_count: int = 0

fn add_variable(name: str) -> int {
    # Add to var_names
    # Return offset: 16 + var_count * 8
}

fn find_variable(name: str) -> int {
    # Search var_names
    # Return offset
}
```

### Phase 3: Binary Expressions (45 min)
**Goal:** Parse and compile `x + 1`, `y - 2`, etc.

Currently: Hardcoded increment

Need to:
1. Parse binary expressions
2. Identify operator (+, -, *, /)
3. Identify operands (variables or literals)
4. Generate appropriate assembly

**Approach:**
```wyn
fn parse_binop(expr: str) -> {left: str, op: str, right: str} {
    # Find operator
    # Split into left and right
}

fn gen_binop(f: str, left: str, op: str, right: str, vars: str) {
    # Load left into x0
    # Load right into x1
    # Apply operation
}
```

### Phase 4: If Statements (30 min)
**Goal:** Compile `if x < 5 { ... }`

Similar to while loops but without the back branch.

### Phase 5: Function Calls (15 min)
**Goal:** Support calls beyond print

Currently: Only print is special-cased

Need to:
1. Detect function calls
2. Generate `bl _function_name`

### Phase 6: Self-Compilation Test (30 min)
**Goal:** Compiler compiles itself

1. Use compiler_v3.wyn as input
2. Generate assembly
3. Compare with stage0 output
4. Debug differences

## Detailed Plan for Phase 1

### Step 1: Detect Assignments (10 min)
```wyn
fn has_assignment(body: str) -> int {
    let pos: int = 0
    while pos < body.len() {
        if body[pos:pos + 1] == "=" {
            # Check it's not == or !=
            if pos > 0 and body[pos-1:pos] != "=" {
                return 1
            }
        }
        pos = pos + 1
    }
    return 0
}
```

### Step 2: Parse Assignment (10 min)
```wyn
fn parse_assignment(body: str) -> {var: str, expr: str} {
    let pos: int = 0
    while pos < body.len() and body[pos:pos + 1] != "=" {
        pos = pos + 1
    }
    
    # Extract variable name (before =)
    let var_end: int = pos - 1
    while var_end > 0 and body[var_end:var_end + 1] == " " {
        var_end = var_end - 1
    }
    let var_start: int = var_end
    while var_start > 0 and body[var_start:var_start + 1] != " " {
        var_start = var_start - 1
    }
    const var: str = body[var_start:var_end + 1]
    
    # Extract expression (after =)
    pos = pos + 1
    while pos < body.len() and body[pos:pos + 1] == " " {
        pos = pos + 1
    }
    let expr_start: int = pos
    while pos < body.len() and body[pos:pos + 1] != "\n" {
        pos = pos + 1
    }
    const expr: str = body[expr_start:pos]
    
    return {var: var, expr: expr}
}
```

### Step 3: Generate Assignment Code (10 min)
```wyn
fn gen_assignment(f: str, var: str, expr: str, var_idx: int) {
    # For now, assume expr is "x + 1"
    const offset: int = 16 + var_idx * 8
    
    io.append_file(f, "    ldr x0, [x29, #")
    io.append_file(f, string.from_int(offset))
    io.append_file(f, "]\n")
    io.append_file(f, "    add x0, x0, #1\n")
    io.append_file(f, "    str x0, [x29, #")
    io.append_file(f, string.from_int(offset))
    io.append_file(f, "]\n")
}
```

## Testing Strategy

After each phase:
1. Create test file
2. Compile with stage0
3. Run and verify output
4. Compare assembly with expected

Example test progression:
- `test_assignment.wyn` - Single variable with assignment
- `test_two_vars.wyn` - Two variables
- `test_expr.wyn` - Binary expressions
- `test_if.wyn` - If statements
- `test_functions.wyn` - Function calls
- `compiler_v3.wyn` - Self-compilation

## Success Criteria

✅ Phase 1: `x = x + 1` compiles correctly
✅ Phase 2: Multiple variables work
✅ Phase 3: `x + y` compiles correctly
✅ Phase 4: `if x < 5` compiles correctly
✅ Phase 5: Function calls work
✅ Phase 6: Compiler compiles itself

## Risk Mitigation

**Risk:** Complex parsing gets stuck
**Mitigation:** Keep parsing simple, hardcode common patterns

**Risk:** Assembly generation errors
**Mitigation:** Test each instruction type separately

**Risk:** Self-compilation fails
**Mitigation:** Start with subset of compiler features

## Timeline

- **Today (Session 3):** Phase 1 + Phase 2 (1 hour)
- **Tomorrow:** Phase 3 + Phase 4 (1.5 hours)
- **Day 3:** Phase 5 + Phase 6 (1 hour)

**Total:** 3.5 hours to full self-hosting

## Current Confidence

- Phase 1: 95% (straightforward)
- Phase 2: 90% (just bookkeeping)
- Phase 3: 80% (more complex parsing)
- Phase 4: 85% (similar to while)
- Phase 5: 90% (simple)
- Phase 6: 70% (unknown unknowns)

**Overall:** 85% confidence in self-hosting within 3 days
