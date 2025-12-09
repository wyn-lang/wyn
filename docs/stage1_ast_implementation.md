# Stage 1 AST-Based Implementation

## Overview

Implemented AST-based code generation pipeline for Wyn Stage 1 compiler self-hosting.

## Components

### 1. codegen.wyn - ARM64 Assembly Generator

**Purpose:** Walk AST and generate ARM64 assembly code

**Key Features:**
- Module/Function/Statement/Expression traversal
- Variable tracking with stack offsets
- String literal collection and generation
- Label management for control flow

**Supported Constructs:**
- Variable declarations (`let`, `const`)
- Binary operations: `+`, `-`, `*`, `/`, `<`, `>`, `==`, `!=`
- Function calls (currently `print()`)
- Control flow: `if/else`, `while` loops
- Return statements

**ARM64 Patterns:**
- Function prologue: `sub sp, sp, #64` / `stp x29, x30, [sp]`
- Variable storage: Stack-relative addressing `[x29, #offset]`
- Binary ops: Stack-based evaluation with `x0`, `x1`
- Comparisons: `cmp` + `cset` for boolean results
- Control flow: Branch instructions `b.eq`, `b.lt`, etc.

**Example Output:**
```asm
_main:
    sub sp, sp, #64
    stp x29, x30, [sp]
    mov x29, sp
    mov x0, #10          ; let x = 10
    str x0, [x29, #16]
    ldr x0, [x29, #16]   ; load x
    mov x1, #5           ; compare with 5
    cmp x0, x1
    cset x0, gt          ; x > 5
    ...
```

### 2. typechecker.wyn - Type Validation

**Purpose:** Validate types in AST before code generation

**Key Features:**
- Variable type tracking
- Expression type inference
- Binary operation type checking
- Error collection

**Type System:**
- `TYPE_INT` - Integer values
- `TYPE_STR` - String values
- `TYPE_VOID` - No return value

**Validation:**
- Variables must be declared before use
- Binary operations require matching types
- Function signatures (basic)

**Usage:**
```wyn
const ctx = check(module)
if has_errors(ctx) == 1 {
    # Handle type errors
}
```

### 3. examples/stage1_ast_test.wyn - Test File

**Purpose:** Comprehensive test for AST-based codegen

**Features Tested:**
- Variable declarations with expressions
- Binary operations: `+`, `-`, `*`
- Comparison operations: `<`, `>`, `==`, `!=`
- Control flow: `if/else`
- Loops: `while` with counter
- Function calls: `print()`

**Test Structure:**
```wyn
fn main() {
    # Variables
    let x: int = 10
    let y: int = 5
    let sum: int = x + y
    
    # Control flow
    if x > y {
        print("x is greater")
    }
    
    # Loops
    let counter: int = 0
    while counter < 3 {
        print("Loop iteration")
        counter = counter + 1
    }
}
```

## Architecture

### Pipeline Flow

```
Source Code
    ↓
Lexer (lexer.wyn)
    ↓
Tokens
    ↓
Parser (parser.wyn)
    ↓
AST
    ↓
TypeChecker (typechecker.wyn)
    ↓
Validated AST
    ↓
CodeGen (codegen.wyn)
    ↓
ARM64 Assembly
    ↓
Assembler (cc)
    ↓
Executable
```

### AST Structure

```wyn
Module
  └─ functions: [Function]
       └─ Function
            ├─ name: str
            └─ body: [Stmt]
                 └─ Stmt
                      ├─ kind: int (LET, EXPR, RETURN, IF, WHILE)
                      ├─ expr: Expr
                      ├─ var_name: str
                      └─ var_value: Expr
                           └─ Expr
                                ├─ kind: int (INT, STRING, IDENT, CALL, BINARY)
                                ├─ int_val: int
                                ├─ str_val: str
                                ├─ left/right: Expr
                                └─ op: str
```

### Code Generation Strategy

**Variable Management:**
- Stack-based allocation starting at `[x29, #16]`
- 8 bytes per variable
- Offset = `16 + (var_index * 8)`

**Expression Evaluation:**
- Result always in `x0`
- Binary ops use stack for left operand
- Right operand in `x1`

**Control Flow:**
- Labels: `L0`, `L1`, `L2`, etc.
- Condition in `x0` (0 = false, non-zero = true)
- Branch on comparison results

**String Literals:**
- Collected during codegen
- Emitted in `.section __TEXT,__cstring`
- Referenced via `@PAGE/@PAGEOFF` pattern

## Current Limitations

1. **No function parameters** - Only `main()` supported
2. **No arrays** - Single values only
3. **No structs** - Primitive types only
4. **Limited operators** - Basic arithmetic and comparison
5. **No for loops** - Only while loops
6. **Single return** - At function end only

## Next Steps for Self-Hosting

1. **Function parameters and calls**
   - Parse parameter lists
   - Generate calling convention code
   - Handle return values

2. **Complex expressions**
   - Operator precedence
   - Parenthesized expressions
   - Nested function calls

3. **Arrays and indexing**
   - Array allocation
   - Index operations
   - Length tracking

4. **String operations**
   - Concatenation
   - Slicing
   - Comparison

5. **Module system**
   - Import resolution
   - Symbol tables
   - Cross-module calls

## Testing

**Compile test file:**
```bash
./build/stage0 -o test examples/stage1_ast_test.wyn
./test
```

**Expected output:**
```
=== Stage 1 AST Test ===
Variables initialized
x is greater
Loop iteration
Loop iteration
Loop iteration
Test complete!
```

## Implementation Notes

**Minimal Design:**
- No complex data structures
- Simple linear scans for lookups
- Direct assembly generation (no IR)
- Stack-based expression evaluation

**ARM64 Specifics:**
- Uses `x0-x1` for operations
- Stack grows downward
- Frame pointer in `x29`
- Link register in `x30`

**Wyn Constraints:**
- No dynamic memory allocation
- Fixed-size arrays
- No recursion in codegen
- Simple string concatenation

## References

- `compiler.wyn` - Original pattern-based compiler
- `lexer.wyn` - Token generation
- `parser.wyn` - AST construction
- `ast.wyn` - AST type definitions
