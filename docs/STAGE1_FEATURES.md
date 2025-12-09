# Stage 1 Compiler - Feature Reference

## Supported Features

### 1. Variables

**Syntax:**
```wyn
let x: int = 0
const name: str = "value"
```

**Implementation:**
- Stored on stack at calculated offsets
- Symbol table tracks variable names
- Offset formula: `16 + (index * 8)`

### 2. Control Flow

**While Loops:**
```wyn
while x < 5 {
    print("loop")
    x = x + 1
}
```

**For Loops:**
```wyn
for i in 0..3 {
    print("iteration")
}
```

**If Statements:**
```wyn
if condition {
    print("true")
}
```

### 3. Functions

**Definition:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

**Features:**
- Multiple parameters
- Return values
- Stack-based calling convention
- ARM64 ABI compliance

### 4. String Variables

**Usage:**
```wyn
const msg: str = "Hello"
print(msg)
```

**Implementation:**
- Strings stored in __cstring section
- Pointers stored on stack
- ARM64 adrp/add addressing

### 5. Imports

**Supported:**
```wyn
import io
import os
import string
```

**Detection only** - not yet generating import code

### 6. Expressions

**Arithmetic:**
```wyn
let result: int = a + b
let sum: int = x + 1
```

**Comparisons:**
```wyn
while x < 5 { }
if x < max { }  # Variable refs in progress
```

## Compilation Paths

The compiler selects a path based on detected features:

```
Source Code
    ↓
Feature Detection
    ↓
    ├─→ Has string vars? → compile_string_var_program()
    ├─→ Has returns?     → compile_func_program()
    ├─→ Has while?       → compile_while_program()
    ├─→ Has for?         → compile_for_program()
    └─→ Simple           → compile_simple_program()
```

## Testing

### Run a Test

```bash
# 1. Compile Stage 1
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# 2. Run Stage 1 (compiles test file)
./build/stage1

# 3. Execute generated code
./build/stage1_output
```

### Available Tests

| Test File | Features Tested |
|-----------|----------------|
| `stage1_func_test.wyn` | Function params, returns |
| `stage1_string_test.wyn` | String variables |
| `stage1_test.wyn` | While loops, multiple vars |
| `stage1_all_features.wyn` | Comprehensive |

## Generated Assembly

### Function with Parameters

**Wyn Code:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

**Generated ARM64:**
```asm
_add:
    sub sp, sp, #32          ; Allocate stack
    stp x29, x30, [sp]       ; Save frame/link
    mov x29, sp              ; Set frame pointer
    str x0, [x29, #16]       ; Store param a
    str x1, [x29, #24]       ; Store param b
    ldr x0, [x29, #16]       ; Load a
    ldr x1, [x29, #24]       ; Load b
    add x0, x0, x1           ; Compute a + b
    ldp x29, x30, [sp]       ; Restore frame/link
    add sp, sp, #32          ; Deallocate stack
    ret                      ; Return (result in x0)
```

### String Variable

**Wyn Code:**
```wyn
const msg: str = "Hello"
print(msg)
```

**Generated ARM64:**
```asm
_main:
    sub sp, sp, #64
    stp x29, x30, [sp]
    mov x29, sp
    adrp x0, msg0@PAGE       ; Load string address (high)
    add x0, x0, msg0@PAGEOFF ; Load string address (low)
    str x0, [x29, #16]       ; Store pointer
    ldr x0, [x29, #16]       ; Load for print
    bl _puts                 ; Call print
    mov x0, #0
    ldp x29, x30, [sp]
    add sp, sp, #64
    ret

.section __TEXT,__cstring
msg0:
    .asciz "Hello"
```

## Limitations

### Not Yet Supported

1. **Function Calls** - Can define but not call user functions
2. **Complex Expressions** - Only simple arithmetic
3. **Variable References** - In comparisons (in progress)
4. **Arrays** - Not implemented
5. **Structs** - Not implemented
6. **Type Inference** - Explicit types required

### Workarounds

**Function Calls:**
- Currently hardcoded for specific test cases
- Need general call parsing

**Complex Expressions:**
- Break into simple statements
- Use temporary variables

## Next Features

### Priority 1: Variable References
```wyn
const max: int = 10
while x < max { }  # Use variable, not literal
```

### Priority 2: Function Calls
```wyn
const result: int = add(x, y)  # Parse and generate call
```

### Priority 3: Expression Trees
```wyn
const result: int = (a + b) * (c - d)  # Nested expressions
```

## Architecture Notes

### Symbol Table
- Simple comma-separated string
- Example: `"x,y,max"`
- Lookup via string scanning

### Stack Layout
```
High Address
+------------------+
| Return Address   | [sp + 8]
| Frame Pointer    | [sp + 0]
+------------------+ <- x29
| Variable 0       | [x29 + 16]
| Variable 1       | [x29 + 24]
| Variable 2       | [x29 + 32]
+------------------+
Low Address
```

### Register Usage
- `x0-x7`: Function parameters
- `x0`: Return value
- `x29`: Frame pointer
- `x30`: Link register (return address)
- `sp`: Stack pointer

## Performance

### Compilation Speed
- Stage 0 → Stage 1: ~1 second
- Stage 1 → Output: ~0.1 seconds
- Total: ~1.1 seconds

### Generated Code
- Unoptimized
- Excessive stack usage
- Many redundant loads/stores
- Optimization planned for later

## Contributing

To add a new feature:

1. Create test file in `examples/`
2. Add detection in `main()`
3. Create `compile_X_program()` function
4. Generate ARM64 assembly
5. Test with Stage 0
6. Update documentation

## References

- [ARM64 ABI](https://developer.arm.com/documentation/)
- [Wyn Language Spec](language.md)
- [Roadmap](ROADMAP.md)
