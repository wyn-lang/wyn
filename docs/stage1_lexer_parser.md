# Stage 1 Lexer and Parser

## Overview

The Stage 1 compiler infrastructure includes a complete lexer and parser that tokenize Wyn source code and build an Abstract Syntax Tree (AST).

## Lexer (`src/stage1/lexer.wyn`)

### Features

- **Tokenizes all Wyn keywords**: `fn`, `let`, `const`, `if`, `while`, `for`, `return`, `import`, `struct`
- **Handles literals**: strings (`"..."`), integers
- **Recognizes operators**: `+`, `-`, `*`, `/`, `<`, `>`, `=`, `->`, `.`
- **Tracks position**: line and column numbers for error reporting
- **Skips comments**: lines starting with `#`

### Token Kinds

| Kind | Value | Description |
|------|-------|-------------|
| 0 | EOF | End of file |
| 1 | IDENT | Identifier |
| 3 | INT | Integer literal |
| 7 | STRING | String literal |
| 10 | FN | `fn` keyword |
| 11 | LET | `let` keyword |
| 12 | RETURN | `return` keyword |
| 13 | IF | `if` keyword |
| 14 | WHILE | `while` keyword |
| 15 | IMPORT | `import` keyword |
| 16 | CONST | `const` keyword |
| 17 | FOR | `for` keyword |
| 18 | STRUCT | `struct` keyword |
| 20-29 | - | Delimiters: `()`, `{}`, `[]`, `:`, `,` |
| 26-36 | - | Operators: `=`, `+`, `-`, `*`, `/`, `<`, `>`, `.`, `->` |

### Usage

```wyn
import lexer

const source: str = "fn main() { print(\"hello\") }"
const tokens: [Token] = lexer.lex(source)
```

## Parser (`src/stage1/parser.wyn`)

### Features

- **Builds proper AST** from token stream
- **Parses function definitions** with parameters and return types
- **Parses variable declarations** (`let`/`const`)
- **Parses expressions**: literals, identifiers, function calls
- **Parses control flow**: `if`, `while`, `for` (structure ready)
- **Handles imports**: skips import statements

### AST Structure

#### Module
```wyn
struct Module {
    functions: [Function]
}
```

#### Function
```wyn
struct Function {
    name: str
    body: [Stmt]
}
```

#### Statement
```wyn
struct Stmt {
    kind: int          # 1=EXPR, 2=LET, 3=RETURN
    expr: Expr         # For EXPR and RETURN
    var_name: str      # For LET
    var_value: Expr    # For LET
}
```

#### Expression
```wyn
struct Expr {
    kind: int          # 1=STRING, 2=INT, 3=IDENT, 4=CALL
    str_val: str       # For STRING
    int_val: int       # For INT
    ident: str         # For IDENT
    func_name: str     # For CALL
    args: [Expr]       # For CALL
}
```

### Usage

```wyn
import parser

const tokens: [Token] = lex(source)
const module: Module = parser.parse(tokens)
```

## Example

Input:
```wyn
fn main() {
    let x: int = 42
    print("hello")
}
```

Lexer output (tokens):
```
FN, IDENT(main), LPAREN, RPAREN, LBRACE,
LET, IDENT(x), COLON, IDENT(int), EQ, INT(42),
IDENT(print), LPAREN, STRING(hello), RPAREN,
RBRACE, EOF
```

Parser output (AST):
```
Module {
  functions: [
    Function {
      name: "main",
      body: [
        Stmt { kind: LET, var_name: "x", var_value: Expr{INT, 42} },
        Stmt { kind: EXPR, expr: Expr{CALL, "print", [Expr{STRING, "hello"}]} }
      ]
    }
  ]
}
```

## Integration

The lexer and parser are designed to integrate with the Stage 1 compiler:

1. **Lexer** converts source text → token stream
2. **Parser** converts token stream → AST
3. **Compiler** walks AST → generates assembly

## Status

- ✅ Lexer: Complete and compiles
- ✅ Parser: Complete and compiles
- 🚧 Integration: Ready for compiler integration
- 🚧 Binary expressions: Structure ready, needs implementation
- 🚧 Control flow: Structure ready, needs implementation

## Next Steps

1. Integrate lexer/parser into main compiler
2. Add binary expression parsing (`x + y`)
3. Add control flow statement parsing
4. Add type checking pass
5. Connect to code generation
