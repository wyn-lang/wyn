# Stage 1 Expression Evaluation

Minimal implementation for self-hosting support.

## Functions

### resolve_var_ref(var_names: str, name: str) -> int
Returns stack offset for a variable name.

```wyn
const offset: int = resolve_var_ref("x,y,max", "y")
# Returns: 24 (16 + 1*8)
```

### codegen_binary_op(op: str, left_val: int, right_val: int, is_left_var: int, is_right_var: int, var_names: str) -> str
Generates ARM64 assembly for binary operations.

**Supported operators:**
- Arithmetic: `+`, `-`, `*`, `/`
- Comparison: `<`, `>`, `==`, `!=`

**Parameters:**
- `op`: Operator string
- `left_val`: Left operand (value or var index)
- `right_val`: Right operand (value or var index)
- `is_left_var`: 1 if left is variable, 0 if literal
- `is_right_var`: 1 if right is variable, 0 if literal
- `var_names`: Symbol table (comma-separated names)

**Example:**
```wyn
# Generate: x + 5
const asm: str = codegen_binary_op("+", 0, 5, 1, 0, "x,y")
# Output:
#   ldr x0, [x29, #16]
#   mov x1, #5
#   add x0, x0, x1
```

### eval_expr_to_register(expr: str, reg: str, var_names: str) -> str
Loads expression result into specified register.

**Example:**
```wyn
# Load variable x into x0
const asm1: str = eval_expr_to_register("x", "x0", "x,y")
# Output: ldr x0, [x29, #16]

# Load literal 42 into x1
const asm2: str = eval_expr_to_register("42", "x1", "x,y")
# Output: mov x1, #42
```

## Usage Pattern

```wyn
# Symbol table
const vars: str = "x,y,max"

# Evaluate: while x < max
const cond_asm: str = codegen_binary_op("<", 0, 2, 1, 1, vars)
# Generates comparison between x and max

# Evaluate: x = x + 1
const load_x: str = eval_expr_to_register("x", "x0", vars)
const add_asm: str = codegen_binary_op("+", 0, 1, 1, 0, vars)
# Generates x + 1 in x0
```

## Stack Layout

Variables stored at `[x29, #offset]`:
- First var: offset 16
- Second var: offset 24
- Third var: offset 32
- Pattern: `16 + (index * 8)`

## Next Steps

For full self-hosting, integrate with:
1. Lexer (tokenize expressions)
2. Parser (build expression AST)
3. Code generator (use these functions)
