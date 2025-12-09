# Stage 1 Compiler Architecture

## Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Stage 1 Compiler                         │
│                                                             │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────┐ │
│  │  Source  │───▶│  Lexer   │───▶│  Parser  │───▶│ AST  │ │
│  │   .wyn   │    │          │    │          │    │      │ │
│  └──────────┘    └──────────┘    └──────────┘    └──────┘ │
│                                                       │     │
│                                                       ▼     │
│                                                  ┌──────────┐│
│                                                  │   Type   ││
│                                                  │ Checker  ││
│                                                  └──────────┘│
│                                                       │     │
│                                                       ▼     │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────┐ │
│  │   ARM64  │◀───│ Assembler│◀───│ CodeGen  │◀───│ AST  │ │
│  │   Binary │    │   (cc)   │    │          │    │      │ │
│  └──────────┘    └──────────┘    └──────────┘    └──────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow

```
examples/stage1_ast_test.wyn
         │
         │ io.read_file()
         ▼
    Source String
         │
         │ lex()
         ▼
    [Token, Token, ...]
         │
         │ parse()
         ▼
    Module {
      functions: [
        Function {
          name: "main"
          body: [Stmt, ...]
        }
      ]
    }
         │
         │ check()
         ▼
    Context {
      vars: [VarInfo, ...]
      errors: []
    }
         │
         │ generate()
         ▼
    Assembly String
         │
         │ io.write_file()
         ▼
    build/output.s
         │
         │ os.system("cc ...")
         ▼
    build/output (executable)
```

## AST Structure

```
Module
  │
  └─ functions: [Function]
       │
       └─ Function
            ├─ name: str
            │
            └─ body: [Stmt]
                 │
                 ├─ Stmt (kind: LET)
                 │    ├─ var_name: str
                 │    └─ var_value: Expr
                 │
                 ├─ Stmt (kind: IF)
                 │    ├─ condition: Expr
                 │    ├─ then_body: [Stmt]
                 │    └─ else_body: [Stmt]
                 │
                 ├─ Stmt (kind: WHILE)
                 │    ├─ condition: Expr
                 │    └─ then_body: [Stmt]
                 │
                 ├─ Stmt (kind: RETURN)
                 │    └─ expr: Expr
                 │
                 └─ Stmt (kind: EXPR)
                      └─ expr: Expr
                           │
                           ├─ Expr (kind: INT)
                           │    └─ int_val: int
                           │
                           ├─ Expr (kind: STRING)
                           │    └─ str_val: str
                           │
                           ├─ Expr (kind: IDENT)
                           │    └─ ident: str
                           │
                           ├─ Expr (kind: CALL)
                           │    ├─ func_name: str
                           │    └─ args: [Expr]
                           │
                           └─ Expr (kind: BINARY)
                                ├─ left: Expr
                                ├─ op: str
                                └─ right: Expr
```

## Code Generation Flow

```
generate(Module)
    │
    ├─ Emit: .section __TEXT,__text
    ├─ Emit: .globl _main
    │
    └─ For each Function:
         │
         ├─ gen_function()
         │    │
         │    ├─ Emit: _main:
         │    ├─ Emit: sub sp, sp, #64
         │    ├─ Emit: stp x29, x30, [sp]
         │    │
         │    └─ For each Stmt:
         │         │
         │         ├─ gen_stmt()
         │         │    │
         │         │    ├─ STMT_LET:
         │         │    │    ├─ gen_expr(var_value)
         │         │    │    └─ Emit: str x0, [x29, #offset]
         │         │    │
         │         │    ├─ STMT_IF:
         │         │    │    ├─ gen_expr(condition)
         │         │    │    ├─ Emit: cmp x0, #0
         │         │    │    ├─ Emit: b.eq L_else
         │         │    │    ├─ gen_stmt(then_body)
         │         │    │    ├─ Emit: b L_end
         │         │    │    ├─ Emit: L_else:
         │         │    │    ├─ gen_stmt(else_body)
         │         │    │    └─ Emit: L_end:
         │         │    │
         │         │    └─ STMT_WHILE:
         │         │         ├─ Emit: L_start:
         │         │         ├─ gen_expr(condition)
         │         │         ├─ Emit: cmp x0, #0
         │         │         ├─ Emit: b.eq L_end
         │         │         ├─ gen_stmt(body)
         │         │         ├─ Emit: b L_start
         │         │         └─ Emit: L_end:
         │         │
         │         └─ gen_expr()
         │              │
         │              ├─ EXPR_INT:
         │              │    └─ Emit: mov x0, #value
         │              │
         │              ├─ EXPR_STRING:
         │              │    ├─ Add to strings[]
         │              │    ├─ Emit: adrp x0, msg@PAGE
         │              │    └─ Emit: add x0, x0, msg@PAGEOFF
         │              │
         │              ├─ EXPR_IDENT:
         │              │    └─ Emit: ldr x0, [x29, #offset]
         │              │
         │              ├─ EXPR_CALL:
         │              │    ├─ gen_expr(args[0])
         │              │    └─ Emit: bl _puts
         │              │
         │              └─ EXPR_BINARY:
         │                   ├─ gen_expr(left)
         │                   ├─ Emit: str x0, [sp, #-16]!
         │                   ├─ gen_expr(right)
         │                   ├─ Emit: mov x1, x0
         │                   ├─ Emit: ldr x0, [sp], #16
         │                   └─ Emit: add/sub/mul/div/cmp x0, x0, x1
         │
         ├─ Emit: mov x0, #0
         ├─ Emit: ldp x29, x30, [sp]
         ├─ Emit: add sp, sp, #64
         └─ Emit: ret
    │
    ├─ Emit: .section __TEXT,__cstring
    │
    └─ For each string in strings[]:
         ├─ Emit: msgN:
         └─ Emit: .asciz "string_value"
```

## Memory Layout

```
Stack Frame (64 bytes):

High Address
    │
    ├─ [x29 + 56]  ─┐
    ├─ [x29 + 48]   │ Available for
    ├─ [x29 + 40]   │ local variables
    ├─ [x29 + 32]   │ (8 bytes each)
    ├─ [x29 + 24]   │
    ├─ [x29 + 16]  ─┘
    ├─ [x29 + 8]   ← x30 (link register)
    ├─ [x29 + 0]   ← x29 (frame pointer)
    │
Low Address

Variable Allocation:
- First variable:  [x29, #16]
- Second variable: [x29, #24]
- Third variable:  [x29, #32]
- ...
- Offset = 16 + (index * 8)
```

## Register Usage

```
ARM64 Registers:

x0  ─ Primary result register
      - Expression evaluation result
      - Function return value
      - First argument to functions

x1  ─ Secondary operand
      - Right side of binary operations
      - Second argument to functions

x29 ─ Frame pointer (FP)
      - Points to current stack frame
      - Used for local variable access

x30 ─ Link register (LR)
      - Return address for function calls
      - Saved/restored in prologue/epilogue

sp  ─ Stack pointer
      - Points to top of stack
      - Grows downward (decreasing addresses)
```

## Type System

```
Type Checking Context:

Context {
  vars: [
    VarInfo {
      name: "x"
      typ: TYPE_INT
    },
    VarInfo {
      name: "message"
      typ: TYPE_STR
    }
  ]
  errors: []
}

Type Inference:
- Literals → Direct type
- Variables → Lookup in vars[]
- Binary ops → Check operand types match
- Function calls → Return type
```

## File Organization

```
src/stage1/
├── ast.wyn              # AST type definitions
├── lexer.wyn            # Tokenization
├── parser.wyn           # AST construction
├── typechecker.wyn      # Type validation
├── codegen.wyn          # ARM64 generation
├── compiler_ast.wyn     # Main driver
├── token.wyn            # Token definitions
└── README_AST.md        # This file

examples/
└── stage1_ast_test.wyn  # Test file

build/
├── stage0               # Bootstrap compiler (C)
├── compiler_ast         # AST compiler (compiled)
├── stage1_ast_output.s  # Generated assembly
└── stage1_ast_output    # Final executable
```

## Compilation Pipeline

```
Step 1: Bootstrap
    stage0 (C compiler) compiles compiler_ast.wyn
    → build/compiler_ast

Step 2: Compilation
    compiler_ast reads stage1_ast_test.wyn
    → Lexer tokenizes source
    → Parser builds AST
    → TypeChecker validates types
    → CodeGen emits assembly
    → Writes build/stage1_ast_output.s

Step 3: Assembly
    cc assembles and links
    → build/stage1_ast_output

Step 4: Execution
    ./build/stage1_ast_output
    → Runs compiled program
```

## Self-Hosting Path

```
Current State:
    stage0 (C) → compiler_ast (Wyn) → test programs

Target State:
    stage0 (C) → stage1 (Wyn) → stage1 (Wyn) → ...
                      │              │
                      └──────────────┘
                      (self-hosting)

Steps:
1. ✅ Implement AST-based codegen
2. ✅ Add type checking
3. 🚧 Add function parameters
4. 🚧 Add complex expressions
5. 📋 Add arrays and structs
6. 📋 Compile lexer.wyn with stage1
7. 📋 Compile parser.wyn with stage1
8. 📋 Compile codegen.wyn with stage1
9. 📋 Full bootstrap achieved
```
