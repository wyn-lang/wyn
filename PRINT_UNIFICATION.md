# Print Function Unification

## Summary

Unified all print functions into a single `print()` that handles all types automatically and supports multiple arguments.

## New Syntax

**Before (verbose):**
```wyn
print_str("Hello")
print(42)
print_newline()
print_str("Value: ")
print(value)
print_newline()
```

**After (simple):**
```wyn
print("Hello")
print(42)
print("Value:", value)  // variadic!
print("")  // empty line
```

## Features

- **Type-aware**: Automatically detects and prints strings, integers, floats
- **Auto-newline**: Adds newline after each print (like Python)
- **Variadic**: Accepts multiple arguments with space separator
- **String concatenation**: `print("PASS " + name)` works correctly
- **Simple**: One function for everything

## Variadic Examples

```wyn
// Multiple arguments
print("Value:", 42)
print("Pi:", 3.14)
print("Name:", name)

// Mixed types
print("Mixed:", 1, 2.5, "test")

// Multiple strings
print("Multiple", "strings", "work", "too")
```

## String Concatenation

```wyn
const name: str = "test"
print("PASS " + name)  // Works!
print("Result: " + int_to_str(42))
```

## Implementation

- `print(value1, value2, ...)` - prints all values with spaces, adds newline
- Compiler detects argument types and calls appropriate runtime functions
- Supports: strings, integers, floats, variables, struct fields, concatenation
- Empty string `print("")` for blank lines

## Migration

- Converted 141 `.wyn` files automatically
- Updated all documentation
- Fixed string concatenation detection in print()
- Backward compatible: old functions still exist in runtime

Matches Python/JavaScript conventions - simple and intuitive.
