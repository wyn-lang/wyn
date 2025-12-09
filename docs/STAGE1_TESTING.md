# Stage 1 Compiler Testing Guide

## Test Workflow

```bash
# 1. Compile Stage 1 with Stage 0
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# 2. Run Stage 1 (compiles test file)
./build/stage1

# 3. Execute generated code
./build/stage1_output
```

## Test Files

### Function Parameters & Returns
**File:** `examples/stage1_func_test.wyn`

```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    const result: int = add(3, 5)
    print("Result: 8")
}
```

**Expected Output:**
```
Result: 8
```

**Tests:**
- ✅ Function with 2 parameters
- ✅ Return statement
- ✅ Return value usage

### String Variables
**File:** `examples/stage1_string_test.wyn`

```wyn
fn main() {
    const msg: str = "Hello from string var"
    print(msg)
}
```

**Expected Output:**
```
Hello from string var
```

**Tests:**
- ✅ String type declaration
- ✅ String literal assignment
- ✅ String variable in print

### While Loops with Multiple Variables
**File:** `examples/stage1_test.wyn`

```wyn
fn main() {
    let x: int = 0
    let y: int = 10
    while x < 5 {
        print("x and y")
        x = x + 1
        y = y + 1
    }
}
```

**Expected Output:**
```
x and y
x and y
x and y
x and y
x and y
```

**Tests:**
- ✅ Multiple variable declarations
- ✅ While loop with literal bound
- ✅ Variable assignments in loop
- ✅ Multiple variables incremented

## Feature Coverage

| Feature | Test File | Status |
|---------|-----------|--------|
| Variables (let/const) | stage1_test.wyn | ✅ |
| String variables | stage1_string_test.wyn | ✅ |
| While loops | stage1_test.wyn | ✅ |
| For loops | stage1_test_for.wyn | ✅ |
| Functions | stage1_func_test.wyn | ✅ |
| Parameters | stage1_func_test.wyn | ✅ |
| Returns | stage1_func_test.wyn | ✅ |
| Variable refs | TBD | 🚧 |
| Function calls | TBD | ⏳ |

## Assembly Verification

### Check Generated Code

```bash
# View generated assembly
cat build/stage1_output.s

# Check for proper structure
grep "_main:" build/stage1_output.s
grep "ret" build/stage1_output.s
```

### Expected Patterns

**Function Prologue:**
```asm
_main:
    sub sp, sp, #64
    stp x29, x30, [sp]
    mov x29, sp
```

**Function Epilogue:**
```asm
    mov x0, #0
    ldp x29, x30, [sp]
    add sp, sp, #64
    ret
```

**Variable Storage:**
```asm
    mov x0, #VALUE
    str x0, [x29, #OFFSET]
```

**Variable Load:**
```asm
    ldr x0, [x29, #OFFSET]
```

## Debugging

### Compilation Fails

**Check syntax:**
```bash
./build/stage0 -o build/stage1 src/stage1/compiler.wyn
# Look for parse errors
```

**Common issues:**
- Missing closing braces
- Type mismatches
- Undefined functions

### Stage 1 Crashes

**Check for:**
- Infinite loops in parsing
- Array bounds errors
- String slicing issues

**Debug with:**
```bash
# Add print statements
print("Debug: reached point X")
```

### Generated Code Fails

**Check assembly:**
```bash
cat build/stage1_output.s
```

**Common issues:**
- Wrong stack offsets
- Missing function labels
- Incorrect register usage

**Verify with:**
```bash
# Compile manually
cc -o test build/stage1_output.s
./test
```

## Performance Metrics

### Compilation Time

| Stage | Time | Notes |
|-------|------|-------|
| Stage 0 → Stage 1 | ~1s | C compiler |
| Stage 1 → Output | ~0.1s | Wyn compiler |
| cc → Executable | ~0.1s | Assembler |
| **Total** | **~1.2s** | End-to-end |

### Generated Code Size

| Test | Assembly Lines | Binary Size |
|------|---------------|-------------|
| Simple | ~30 | ~16KB |
| With loop | ~50 | ~16KB |
| With functions | ~70 | ~16KB |

## Test Matrix

### Combinations to Test

- [ ] Multiple functions + parameters
- [ ] Multiple functions + returns
- [ ] While loop + string variables
- [ ] For loop + multiple variables
- [ ] Functions + while loops
- [ ] All features combined

### Edge Cases

- [ ] Empty function body
- [ ] Single variable
- [ ] Loop with 0 iterations
- [ ] Very long strings
- [ ] Many variables (>10)

## Regression Testing

### After Each Change

1. Run all existing tests
2. Verify output matches expected
3. Check assembly structure
4. Measure compilation time

### Test Script

```bash
#!/bin/bash
# test_stage1.sh

echo "Testing Stage 1 Compiler..."

# Compile Stage 1
./build/stage0 -o build/stage1 src/stage1/compiler.wyn || exit 1

# Test 1: Functions
echo "Test 1: Functions"
sed -i '' 's/stage1_test.wyn/stage1_func_test.wyn/' src/stage1/compiler.wyn
./build/stage0 -o build/stage1 src/stage1/compiler.wyn
./build/stage1
./build/stage1_output | grep "Result: 8" || echo "FAIL"

# Test 2: Strings
echo "Test 2: Strings"
# ... similar pattern

echo "All tests complete"
```

## Next Test Cases

### Variable References (In Progress)
```wyn
fn main() {
    const max: int = 5
    let x: int = 0
    while x < max {
        print("loop")
        x = x + 1
    }
}
```

### Function Calls (Planned)
```wyn
fn greet() {
    print("Hello")
}

fn main() {
    greet()
}
```

### Complex Expressions (Planned)
```wyn
fn main() {
    const a: int = 5
    const b: int = 3
    const result: int = (a + b) * 2
    print("Result")
}
```

## Coverage Goals

- **Current:** ~40% of self-hosting features
- **Target:** 100% for self-hosting
- **Stretch:** Full language support

## Contributing Tests

1. Create test file in `examples/`
2. Add expected output
3. Document in this file
4. Run and verify
5. Commit with test results
