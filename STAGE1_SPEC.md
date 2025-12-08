# Stage 1 Compiler - Complete Specification

## Goal

Build a fully self-hosting Wyn compiler written in Wyn that supports all core language features.

## Current State

✅ **Proof of concept working** (100 lines)
- Compiles hello world
- Generates ARM64 assembly
- Creates working binaries

## Target Features

### 1. Core Language Support

**Types:**
- `int`, `str`, `bool`
- Arrays: `[int]`, `[str]`
- Structs

**Functions:**
- Parameters and return types
- Local variables
- Function calls

**Expressions:**
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `and`, `or`, `not`
- String concatenation: `+`
- Array indexing: `arr[i]`
- Field access: `obj.field`
- Method calls: `arr.len()`, `str.upper()`

**Statements:**
- `let` and `const` declarations
- Assignment: `x = value`
- `return`
- `if`/`else`
- `while` loops
- `for` loops
- Function calls

**Imports:**
- `import io`
- `import time`
- Module function calls: `io.read_file()`

### 2. Builtins (8 only)

1. `print()` - Output
2. `assert()` - Testing
3. `exit()` - Process control
4. `args()` - Command line arguments
5. `int_to_str()` - String conversion
6. `system()` - Shell commands
7. `write_file()` - File writing
8. `substring()` - String slicing
9. `ord()` - Character code

### 3. Code Generation

**Target:** ARM64 assembly for macOS

**Must generate:**
- Function prologues/epilogues
- Variable storage (stack)
- Expression evaluation
- Control flow (branches, loops)
- Function calls
- Builtin calls
- Module function calls

## Implementation Plan

### Phase 1: Lexer (Complete) ✅

**File:** `src/stage1/lexer.wyn`

**Features:**
- Tokenize all Wyn syntax
- Handle strings, numbers, identifiers, keywords
- Track line/column for errors

**Status:** Basic version exists

### Phase 2: Parser (10-15 hours)

**File:** `src/stage1/parser.wyn`

**Must parse:**
```wyn
# Imports
import io

# Functions
fn add(a: int, b: int) -> int {
    return a + b
}

# Variables
let x: int = 5
const name: str = "Alice"

# Expressions
x = x + 1
const result = add(x, 10)

# Control flow
if x > 0 {
    print(x)
}

while x < 10 {
    x = x + 1
}

for i in 0..10 {
    print(i)
}

# Arrays
const arr: [int] = [1, 2, 3]
print(arr[0])
print(arr.len())

# Structs
struct Point {
    x: int
    y: int
}

const p: Point = Point{x: 3, y: 4}
print(p.x)
```

**Output:** AST (Abstract Syntax Tree)

**Data structures:**
```wyn
struct Module {
    imports: [Import]
    functions: [Function]
    structs: [Struct]
}

struct Function {
    name: str
    params: [Param]
    return_type: str
    body: [Stmt]
}

struct Stmt {
    kind: int  # LET, ASSIGN, RETURN, IF, WHILE, FOR, EXPR
    # ... fields for each kind
}

struct Expr {
    kind: int  # INT, STRING, IDENT, BINARY, CALL, INDEX, FIELD
    # ... fields for each kind
}
```

### Phase 3: Type Checker (5-10 hours)

**File:** `src/stage1/typechecker.wyn`

**Must check:**
- Variable declarations have types
- Function calls match signatures
- Return types match
- Array indexing is valid
- Imports are valid

**Can be simple:** Just validate, don't infer complex types.

### Phase 4: Code Generator (15-20 hours)

**File:** `src/stage1/codegen.wyn`

**Must generate ARM64 for:**

**Functions:**
```asm
_function_name:
    sub sp, sp, #4096
    stp x29, x30, [sp]
    mov x29, sp
    # ... body
    ldp x29, x30, [sp]
    add sp, sp, #4096
    ret
```

**Variables:**
```asm
# let x: int = 5
mov x0, #5
str x0, [x29, #offset]
```

**Expressions:**
```asm
# a + b
ldr x0, [x29, #offset_a]
ldr x1, [x29, #offset_b]
add x0, x0, x1
```

**Function calls:**
```asm
# add(5, 3)
mov x0, #5
mov x1, #3
bl _add
```

**Control flow:**
```asm
# if x > 0 { ... }
ldr x0, [x29, #offset_x]
cmp x0, #0
b.le L_else
# ... then block
b L_end
L_else:
# ... else block
L_end:
```

**Builtins:**
```asm
# print("hello")
adrp x0, str@PAGE
add x0, x0, str@PAGEOFF
bl _puts
```

**Module calls:**
```asm
# io.read_file("file.txt")
adrp x0, str@PAGE
add x0, x0, str@PAGEOFF
bl _read_file
```

### Phase 5: Integration (3-5 hours)

**File:** `src/stage1/compiler.wyn` (main)

**Flow:**
```wyn
fn main() {
    # Parse args
    const input_file = args()[1]
    const output_file = args()[2]
    
    # Read source
    const source = io.read_file(input_file)
    
    # Compile
    const tokens = lex(source)
    const ast = parse(tokens)
    typecheck(ast)
    const asm = codegen(ast)
    
    # Write and assemble
    io.write_file(output_file + ".s", asm)
    system("cc -o " + output_file + " " + output_file + ".s")
}
```

### Phase 6: Self-Hosting (5-10 hours)

**Test:**
```bash
# Compile Stage 1 with stage0
./build/stage0 -o stage1_v1 src/stage1/compiler.wyn

# Compile Stage 1 with itself
./stage1_v1 -o stage1_v2 src/stage1/compiler.wyn

# Verify they work the same
./stage1_v2 -o hello hello.wyn
./hello
```

**Requirements:**
- Stage 1 must support all features it uses
- Output must be correct
- Must be able to compile itself repeatedly

## Timeline

| Phase | Hours | Description |
|-------|-------|-------------|
| Lexer | ✅ Done | Tokenization |
| Parser | 10-15 | Full AST building |
| Type Checker | 5-10 | Validation |
| Codegen | 15-20 | ARM64 generation |
| Integration | 3-5 | Wire together |
| Self-hosting | 5-10 | Bootstrap |
| **Total** | **38-60 hours** | **5-8 days** |

## Success Criteria

✅ Stage 1 compiles hello.wyn
✅ Stage 1 compiles programs with variables
✅ Stage 1 compiles programs with functions
✅ Stage 1 compiles programs with control flow
✅ Stage 1 compiles programs with imports
✅ Stage 1 compiles itself
✅ Stage 1 output matches stage0 output

## Workarounds for stage0 Bugs

**Known bugs in stage0:**
- String concatenation in functions returns garbage
- substring() returns wrong values
- args() indexing beyond index 1 broken

**Workarounds:**
- Use io.append_file() instead of string concat
- Hardcode values instead of parsing
- Use global variables instead of function returns
- Keep Stage 1 simple initially

**These bugs will be fixed once Stage 1 is self-hosting.**

## File Structure

```
src/stage1/
  compiler.wyn      # Main entry point (200 lines)
  lexer.wyn         # Tokenizer (300 lines)
  parser.wyn        # AST builder (800 lines)
  typechecker.wyn   # Validator (400 lines)
  codegen.wyn       # ARM64 generator (1,200 lines)
  ast.wyn           # AST data structures (200 lines)
  
Total: ~3,100 lines of Wyn code
```

## Testing Strategy

**Incremental testing:**
1. Test lexer with sample code
2. Test parser with sample code
3. Test codegen with simple programs
4. Test full compiler with hello.wyn
5. Test with more complex programs
6. Test self-compilation

**Test programs:**
- hello.wyn (print)
- variables.wyn (let/const)
- functions.wyn (function calls)
- control.wyn (if/while/for)
- imports.wyn (module system)
- stage1.wyn (self-compilation)

## After Self-Hosting

Once Stage 1 compiles itself:

### Phase 7: Fix stage0 Bugs (5-10 hours)
- Fix string handling in Stage 1
- Fix array handling in Stage 1
- Now Stage 1 is bug-free

### Phase 8: Add Syscalls (10-15 hours)
- Add `syscall()` primitive to Stage 1
- Rewrite stdlib with syscalls
- Remove C runtime
- Pure Wyn

### Phase 9: Optimize (10-20 hours)
- Add optimizations
- Better error messages
- Faster compilation

## Final Result

**A fully self-hosting, pure Wyn compiler:**
- Written in Wyn
- Compiles itself
- No C dependencies (syscalls only)
- Native performance
- Clean, maintainable codebase

**Total time: 63-105 hours (2-3 weeks of focused work)**

## Immediate Next Steps

1. **Expand parser** (10-15 hours)
   - Parse all statement types
   - Parse all expression types
   - Build complete AST

2. **Expand codegen** (15-20 hours)
   - Generate code for all features
   - Handle variables, functions, control flow
   - Module system

3. **Test and debug** (5-10 hours)
   - Compile test programs
   - Fix bugs
   - Verify correctness

4. **Self-hosting** (5-10 hours)
   - Compile Stage 1 with Stage 1
   - Verify output
   - Bootstrap complete

**Total: 35-55 hours to self-hosting**

Then syscalls and pure Wyn (10-15 hours).

**Grand total: 45-70 hours to fully self-hosting pure Wyn compiler.**
