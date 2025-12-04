# Stage 1 Parser Implementation

## Overview

The Stage 1 parser is a recursive descent parser with precedence climbing for expressions. It transforms a stream of tokens into an Abstract Syntax Tree (AST).

## Status

✅ **Design Complete** - Full specification documented in `src/stage1/parser.wyn`  
⏳ **Implementation** - To be implemented in C or once Stage 1 is bootstrapped  
📚 **Reference** - See `bootstrap/stage0.c` lines 595-1800 for working implementation

## AST Node Types

### Type Kinds (TypeKind enum)

**Primitives:**
- `INT`, `FLOAT`, `STR`, `BOOL`, `BYTE`, `ANY`, `VOID`

**Collections:**
- `ARRAY` - `[T]`
- `MAP` - `{K: V}`
- `TUPLE` - `(T, U, V)`

**Error Handling:**
- `OPTIONAL` - `?T`
- `RESULT` - `Result[T, E]`

**Complex Types:**
- `FUNCTION` - `fn(T) -> U`
- `NAMED` - User-defined types
- `GENERIC` - `Name[T]`
- `POINTER` - `*T`

### Expression Kinds (ExprKind enum)

**Literals:**
- `INT`, `FLOAT`, `STRING`, `BOOL`, `IDENT`

**Operators:**
- `BINARY` - Binary operations (a + b, a * b, etc.)
- `UNARY` - Unary operations (-, not, !, &, *)

**Access:**
- `CALL` - Function calls `f(x)`
- `INDEX` - Array/map access `a[i]`
- `FIELD` - Struct field access `obj.field`

**Constructors:**
- `ARRAY` - `[1, 2, 3]`
- `MAP` - `{"key": value}`
- `TUPLE` - `(1, "two", 3.0)`
- `STRUCT` - `Point{x: 1, y: 2}`
- `LAMBDA` - `fn(x) { x + 1 }`

**Control Flow:**
- `IF` - If expressions `if cond { a } else { b }`
- `MATCH` - Pattern matching

**Option/Result:**
- `SOME`, `NONE` - Option constructors
- `OK`, `ERR` - Result constructors
- `UNWRAP` - `value!`
- `TRY` - `value?`
- `DEFAULT` - `value ?? default`

**Advanced:**
- `SLICE` - `arr[start..end]`
- `ADDR` - `&value`
- `DEREF` - `*ptr`

### Statement Kinds (StmtKind enum)

**Variables:**
- `LET` - Immutable binding `let x = 5`
- `ASSIGN` - Assignment `x = 10`, `x += 5`

**Control Flow:**
- `IF` - If statement
- `WHILE` - While loop
- `FOR` - For loop
- `MATCH` - Match statement

**Function Control:**
- `RETURN` - Return from function
- `BREAK` - Break from loop
- `CONTINUE` - Continue loop

**Other:**
- `EXPR` - Expression statement
- `BLOCK` - Statement block `{ ... }`
- `DEFER` - Deferred execution
- `SPAWN` - Spawn concurrent task

## Operator Precedence

From highest to lowest:

| Level | Operators | Description |
|-------|-----------|-------------|
| 11 | `**` | Exponentiation |
| 10 | `*` `/` `%` | Multiplication, division, modulo |
| 9 | `+` `-` | Addition, subtraction |
| 8 | `<<` `>>` | Bit shifts |
| 7 | `&` | Bitwise AND |
| 6 | `^` | Bitwise XOR |
| 5 | `\|` | Bitwise OR |
| 4 | `<` `>` `<=` `>=` | Comparison |
| 3 | `==` `!=` | Equality |
| 2 | `and` `&&` | Logical AND |
| 1 | `or` `\|\|` | Logical OR |
| 0 | `??` | Null coalescing |

## Parsing Functions

### Top Level

**`parse_module() -> Module`**
- Parses entire source file
- Returns module with imports, functions, structs, enums

### Declarations

**`parse_function(is_pub: bool) -> FnDef`**
```wyn
fn name[T](param: Type) -> ReturnType {
    body
}
```

**`parse_struct(is_pub: bool) -> StructDef`**
```wyn
struct Name[T] {
    field: Type
}
```

**`parse_enum(is_pub: bool) -> EnumDef`**
```wyn
enum Name[T] {
    Variant
    VariantWithData(Type)
}
```

### Statements

**`parse_stmt() -> Stmt`**

Handles:
- `let x = value` - Variable declaration
- `mut x = value` - Mutable variable
- `x = value` - Assignment
- `x += value` - Compound assignment
- `if cond { ... }` - If statement
- `while cond { ... }` - While loop
- `for x in iter { ... }` - For loop
- `match value { ... }` - Match statement
- `return value` - Return
- `break`, `continue` - Loop control
- `defer expr` - Deferred execution
- `{ ... }` - Block
- `expr` - Expression statement

### Expressions

**`parse_expr() -> Expr`**
- Entry point for expression parsing
- Calls `parse_binary(0)`

**`parse_binary(min_prec: int) -> Expr`**
- Precedence climbing algorithm
- Handles all binary operators

**`parse_unary() -> Expr`**
- Handles: `-`, `not`, `!`, `&`, `*`

**`parse_postfix() -> Expr`**
- Handles: `()`, `[]`, `.`, `!`, `?`

**`parse_primary() -> Expr`**
- Literals: `42`, `3.14`, `"hello"`, `true`, `false`
- Identifiers: `variable_name`
- Parentheses: `(expr)`
- Arrays: `[1, 2, 3]`
- Maps: `{"key": value}`
- If expressions: `if cond { a } else { b }`
- Match expressions
- Option/Result constructors: `some(x)`, `none`, `ok(x)`, `err(e)`

### Types

**`parse_type() -> Type`**

Handles:
- Primitives: `int`, `float`, `str`, `bool`, `byte`, `any`
- Arrays: `[Type]`
- Maps: `{KeyType: ValueType}`
- Tuples: `(Type1, Type2)`
- Optional: `?Type`
- Result: `Result[OkType, ErrType]`
- Function: `fn(ParamType) -> ReturnType`
- Pointer: `*Type`
- Named: `TypeName`
- Generic: `TypeName[T, U]`

## Error Reporting

Errors include:
- **File name**
- **Line number**
- **Column number**
- **Error message**

Example:
```
src/main.wyn:10:5: error: expected '}', got 'EOF'
```

## Example Parse Tree

**Input:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

**Parse Tree:**
```
FnDef {
    name: "add"
    params: ["a", "b"]
    param_types: [Type{INT}, Type{INT}]
    return_type: Type{INT}
    body: [
        Stmt{RETURN,
            ret_value: Expr{BINARY,
                left: Expr{IDENT, "a"},
                op: PLUS,
                right: Expr{IDENT, "b"}
            }
        }
    ]
}
```

## Implementation Notes

1. **Recursive Descent** - Each grammar rule becomes a function
2. **Precedence Climbing** - Efficient binary operator parsing
3. **Error Recovery** - Continue parsing after errors when possible
4. **Token Lookahead** - Single token lookahead for LL(1) parsing
5. **Left Recursion** - Handled via iteration in postfix operators

## Testing

Test the parser by:
1. Parsing simple programs
2. Verifying AST structure
3. Testing error reporting
4. Checking all syntax forms

Example test:
```bash
# Parse a simple program
./build/stage0 -o test examples/hello.wyn

# Should output AST structure
```

## Next Steps

1. ✅ Design complete
2. ⏳ Implement in C or wait for Stage 1 bootstrap
3. ⏳ Add comprehensive tests
4. ⏳ Integrate with lexer and type checker
5. ⏳ Add semantic analysis

## References

- **Stage 0 Implementation:** `bootstrap/stage0.c` lines 595-1800
- **Token Definitions:** `src/stage1/token.wyn`
- **Parser Design:** `src/stage1/parser.wyn`
- **Language Spec:** `docs/2-language-spec.md`
