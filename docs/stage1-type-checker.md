# Stage 1 Type Checker

The Stage 1 type checker is an enhanced version of the bootstrap compiler's type checker with improved error messages, type inference, and better scope tracking.

## Features

### 1. Type Inference
The Stage 1 type checker can infer types where possible, reducing the need for explicit type annotations:

```wyn
let x = 42          # Inferred as int
let y = 3.14        # Inferred as float
let s = "hello"     # Inferred as str
let arr = [1, 2, 3] # Inferred as [int]
```

### 2. Better Error Messages
Error messages include helpful suggestions for common mistakes:

```wyn
const x: int = 42
const y: float = 3.14
let z = x + y  # Error: type mismatch: int vs float
               # suggestion: convert int to float with .to_float()
```

### 3. Mutability Checking
Clear errors when trying to modify immutable variables:

```wyn
let immutable = 10
immutable = 20  # Error: cannot assign to immutable variable 'immutable'
                # suggestion: declare with 'let' to make it mutable
```

### 4. Proper Scope Tracking
Variables are tracked across scopes with up to 2048 symbols (vs 1024 in Stage 0):

```wyn
let outer = 10
if true {
    let inner = 20
    let sum = outer + inner  # OK
}
# inner not accessible here
```

### 5. Enhanced Type Validation
- Array indexing must use int type
- Conditions must be bool type
- Binary operations validate operand types
- Method vs field access properly distinguished

## Usage

Enable the Stage 1 type checker with the `--stage1-tc` flag:

```bash
./build/stage0 --stage1-tc -o output input.wyn
```

## Examples

### Type Mismatch with Suggestion
```wyn
const x: int = 42
const y: float = 3.14
let z = x + y
```

**Error:**
```
error: type mismatch: int vs float
  suggestion: convert int to float with .to_float()
```

### Invalid Array Index
```wyn
let arr = [1, 2, 3]
const idx: str = "hello"
let elem = arr[idx]
```

**Error:**
```
error: index must be int, got str
```

### Non-Boolean Condition
```wyn
let x = 42
if x {
    print_str("test")
}
```

**Error:**
```
error: condition must be bool, got int
```

## Implementation Details

### Architecture
- **TypeChecker1**: Enhanced type checker structure
- **Symbol Table**: 2048 entries (2x Stage 0)
- **Type Inference**: Enabled by default
- **Error Context**: Line and column information

### Key Functions
- `tc1_new()`: Create new type checker instance
- `tc1_check_expr()`: Type check expressions with inference
- `tc1_check_stmt()`: Type check statements with scope tracking
- `tc1_error()`: Report errors with location
- `tc1_suggest()`: Provide helpful suggestions

### Modular Design
The Stage 1 type checker is implemented as a separate module in `bootstrap/stage0.c` that can be enabled via command-line flag. It's fully compatible with the existing code generator.

## Testing

All 47 tests from the test suite pass with the Stage 1 type checker:

```bash
# Run tests with Stage 1 type checker
./build/stage0 --stage1-tc -o test_output tests/test_file.wyn
```

## Comparison with Stage 0

| Feature | Stage 0 | Stage 1 |
|---------|---------|---------|
| Symbol Table Size | 1024 | 2048 |
| Type Inference | Limited | Enhanced |
| Error Messages | Basic | With suggestions |
| Scope Tracking | Basic | Enhanced |
| Mutability Checks | Basic | With suggestions |
| Method Detection | Basic | Enhanced |

## Future Enhancements

Potential improvements for Stage 2:
- Generic type inference
- More sophisticated type unification
- Flow-sensitive type analysis
- Exhaustiveness checking for match expressions
- Unused variable warnings
- Dead code detection

## Performance

The Stage 1 type checker has minimal performance overhead compared to Stage 0:
- Compilation time: ~same as Stage 0
- Memory usage: Slightly higher due to larger symbol table
- All tests complete in <3 minutes

## Compatibility

The Stage 1 type checker is fully backward compatible with Stage 0:
- Same AST structure
- Same code generation
- Can be toggled via flag
- No changes to language semantics
