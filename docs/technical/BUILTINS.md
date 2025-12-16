# Wyn Builtins

## Philosophy

Wyn provides a minimal set of builtin functions that are available without imports. These are fundamental operations needed by almost every program.

## Available Without Import

### Output
- `print(value)` - Print value with newline
- `println(value)` - Alias for print (for compatibility)

### Assertions
- `assert(condition: bool)` - Panic if condition is false

### String Operations
- `ord(s: str) -> int` - Get ASCII code of first character
- `chr(code: int) -> str` - Convert ASCII code to character
- `str_to_int(s: str) -> int` - Parse string as integer
- `int_to_str(n: int) -> str` - Convert integer to string
- `str_len(s: str) -> int` - Get string length
- `str_find(s: str, substr: str) -> int` - Find substring position
- `str_concat(a: str, b: str) -> str` - Concatenate strings
- `str_cmp(a: str, b: str) -> int` - Compare strings
- `substring(s: str, start: int, len: int) -> str` - Extract substring
- `char_at(s: str, index: int) -> str` - Get character at index

### String Methods
- `s.len()` - String length (syntactic sugar for str_len)

### Array Methods
- `arr.len()` - Array length

## Require Import

### String Module (`import string`)
- `upper(s: str) -> str`
- `lower(s: str) -> str`
- `trim(s: str) -> str`
- `starts_with(s: str, prefix: str) -> bool`
- `ends_with(s: str, suffix: str) -> bool`
- `contains(s: str, substr: str) -> bool`
- `find(s: str, substr: str) -> int`
- `split(s: str, delimiter: str) -> [str]`

### Math Module (`import math`)
- Trigonometric functions
- Logarithms
- Power functions
- Constants

### Array Module (`import array`)
- Advanced array operations
- Functional programming helpers

### Task Module (`import task`)
- Concurrency primitives
- Channels
- Async operations

## Rationale

### Why These Builtins?

1. **print/println**: Every program needs output
2. **assert**: Essential for testing and debugging
3. **String basics**: String manipulation is fundamental
4. **String methods**: Convenient syntax for common operations

### Why Not More?

- Keeps language core minimal
- Encourages explicit dependencies
- Makes code more maintainable
- Follows principle: "import what you use"

## Future Considerations

As the language matures, we may:
- Move some string functions to require `import string`
- Add more method syntax (`.upper()`, `.lower()`, etc.)
- Introduce a prelude module that's auto-imported

For now, the current set provides a good balance between convenience and explicitness.

## Examples

### No Imports Needed
```wyn
fn main() {
    print("Hello")
    assert(5 > 3)
    
    const s: str = "test"
    print(s.len())
    print(ord("A"))
    print(chr(65))
}
```

### With Imports
```wyn
import string
import math

fn main() {
    const s: str = string.upper("hello")
    print(s)
    
    const x: float = math.sin(3.14)
    print(x)
}
```
