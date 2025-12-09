# Stage 1 AST-Based Compiler Implementation

## Overview

Implemented AST-based compilation pipeline for Wyn Stage 1 self-hosting compiler.

## Files Created

### Core Components

1. **codegen.wyn** - AST to ARM64 assembly generator
   - Walks Module → Functions → Statements → Expressions
   - Generates ARM64 assembly for all language constructs
   - Handles variables, expressions, control flow, function calls
   - Stack-based variable allocation
   - String literal collection and emission

2. **typechecker.wyn** (enhanced) - Type validation
   - Variable type tracking
   - Expression type inference
   - Binary operation type checking
   - Error collection and reporting

3. **compiler_ast.wyn** - Main AST compiler driver
   - Demonstrates full pipeline: Lex → Parse → Check → Generate
   - Integrates all components
   - Handles file I/O and assembly

### Test Files

4. **examples/stage1_ast_test.wyn** - Comprehensive test
   - Variable declarations with expressions
   - Binary operations: +, -, *, /
   - Comparison operations: <, >, ==, !=
   - Control flow: if/else statements
   - Loops: while with counter
   - Function calls: print()

5. **integration_example.wyn** - Pipeline documentation
   - Shows how components connect
   - Documents the compilation flow

### Documentation

6. **docs/stage1_ast_implementation.md** - Full documentation
   - Architecture overview
   - Component descriptions
   - ARM64 code generation patterns
   - AST structure
   - Testing instructions

## Quick Start

### Compile the AST compiler:
```bash
./build/stage0 -o build/compiler_ast src/stage1/compiler_ast.wyn
```

### Run the AST compiler:
```bash
./build/compiler_ast
```

### Run the generated output:
```bash
./build/stage1_ast_output
```

Expected output:
```
AST-based compilation works!
```

## Architecture

```
Source Code (examples/stage1_ast_test.wyn)
    ↓
Lexer (lexer.wyn)
    ↓
Tokens
    ↓
Parser (parser.wyn)
    ↓
AST (Module with Functions)
    ↓
TypeChecker (typechecker.wyn)
    ↓
Validated AST
    ↓
CodeGen (codegen.wyn)
    ↓
ARM64 Assembly (.s file)
    ↓
Assembler (cc)
    ↓
Executable
```

## Implemented Features

### Code Generation (codegen.wyn)

✅ **Statements:**
- Variable declarations (let, const)
- Expression statements
- Return statements
- If/else statements
- While loops

✅ **Expressions:**
- Integer literals
- String literals
- Variable references
- Function calls
- Binary operations

✅ **Binary Operators:**
- Arithmetic: +, -, *, /
- Comparison: <, >, ==, !=

✅ **ARM64 Patterns:**
- Function prologue/epilogue
- Stack-based variable storage
- Expression evaluation
- Control flow with labels
- String literal emission

### Type Checking (typechecker.wyn)

✅ **Features:**
- Variable type tracking
- Expression type inference
- Binary operation validation
- Error collection

✅ **Types:**
- TYPE_INT - Integer values
- TYPE_STR - String values
- TYPE_VOID - No return value

## Code Generation Examples

### Variable Declaration
```wyn
let x: int = 10
```
Generates:
```asm
mov x0, #10
str x0, [x29, #16]
```

### Binary Operation
```wyn
let sum: int = x + y
```
Generates:
```asm
ldr x0, [x29, #16]    ; load x
str x0, [sp, #-16]!   ; save to stack
ldr x0, [x29, #24]    ; load y
mov x1, x0            ; move to x1
ldr x0, [sp], #16     ; restore x from stack
add x0, x0, x1        ; x + y
str x0, [x29, #32]    ; store sum
```

### While Loop
```wyn
while counter < 3 {
    print("Loop")
    counter = counter + 1
}
```
Generates:
```asm
L0:
    ldr x0, [x29, #16]    ; load counter
    mov x1, #3
    cmp x0, x1
    cset x0, lt           ; counter < 3
    cmp x0, #0
    b.eq L1               ; exit if false
    ; loop body
    b L0                  ; repeat
L1:
```

## Current Limitations

1. **No function parameters** - Only main() supported
2. **No arrays** - Single values only
3. **No structs** - Primitive types only
4. **Limited operators** - Basic arithmetic and comparison
5. **No for loops** - Only while loops implemented
6. **Single return** - At function end only

## Next Steps for Self-Hosting

### Phase 1: Function Support
- [ ] Parse function parameters
- [ ] Generate calling convention code
- [ ] Handle multiple return statements
- [ ] Function-to-function calls

### Phase 2: Complex Expressions
- [ ] Operator precedence
- [ ] Parenthesized expressions
- [ ] Nested function calls
- [ ] Assignment operators (+=, -=, etc.)

### Phase 3: Data Structures
- [ ] Array allocation and indexing
- [ ] Struct definitions and access
- [ ] String operations (concat, slice)

### Phase 4: Module System
- [ ] Import resolution
- [ ] Symbol tables
- [ ] Cross-module calls
- [ ] Standard library integration

### Phase 5: Self-Hosting
- [ ] Compile lexer.wyn with Stage 1
- [ ] Compile parser.wyn with Stage 1
- [ ] Compile codegen.wyn with Stage 1
- [ ] Full bootstrap: Stage 1 compiles itself

## Testing

### Test the components:
```bash
# Compile test file
./build/stage0 -o test examples/stage1_ast_test.wyn
./test

# Expected output:
# === Stage 1 AST Test ===
# Variables initialized
# x is greater
# Loop iteration
# Loop iteration
# Loop iteration
# Test complete!
```

### Verify codegen:
```bash
# Check generated assembly
cat build/stage1_ast_output.s
```

## Implementation Notes

### Design Principles
- **Minimal:** No complex data structures
- **Direct:** No intermediate representation
- **Simple:** Linear scans for lookups
- **Stack-based:** Expression evaluation

### ARM64 Specifics
- Registers: x0-x1 for operations
- Stack: Grows downward
- Frame pointer: x29
- Link register: x30
- Offsets: 16 + (index * 8)

### Wyn Constraints
- No dynamic allocation
- Fixed-size arrays
- No recursion in codegen
- Simple string operations

## References

- **ast.wyn** - AST type definitions
- **lexer.wyn** - Tokenization
- **parser.wyn** - AST construction
- **compiler.wyn** - Original pattern-based compiler
- **docs/stage1_ast_implementation.md** - Detailed documentation

## Status

✅ **Completed:**
- AST-based code generation
- Type checking framework
- Test file with comprehensive examples
- Documentation

🚧 **In Progress:**
- Full lexer/parser integration
- Function parameters and calls
- Complex expression handling

📋 **Planned:**
- Arrays and structs
- Module system
- Self-hosting capability
