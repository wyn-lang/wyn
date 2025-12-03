# Module System Implementation

## Overview
Implemented a module/namespace system to organize builtins into logical modules.

## Changes Made

### 1. Module Structure
Created three core modules in `std/`:
- `std/io.wyn` - I/O operations (print, println, input, etc.)
- `std/fs.wyn` - File system operations (read_file, write_file, etc.)
- `std/math.wyn` - Math functions (sin, cos, sqrt, etc.)

### 2. Syntax
- Module functions: `io.print("hello")`, `fs.read_file("path")`, `math.sin(1.0)`
- Methods unchanged: `"hello".len()`, `(-5).abs()`, `[1,2,3].contains(2)`

### 3. Compiler Changes
Modified `bootstrap/stage0.c`:
- Added support for `module.function()` syntax via EXPR_FIELD
- Updated builtin recognition to handle both `io.print` and legacy `print`
- Maintained backward compatibility during transition

### 4. Test Updates
Updated all 16 test files to use new module syntax:
- `tests/builtins_test.wyn`
- `tests/fs_test.wyn`
- `tests/math_stdlib_test.wyn`
- `tests/string_test.wyn`
- `tests/string_funcs_test.wyn`
- `tests/string_stdlib_test.wyn`
- `tests/arithmetic_test.wyn`
- `tests/array_test.wyn`
- `tests/bitwise_test.wyn`
- `tests/comparison_test.wyn`
- `tests/control_flow_test.wyn`
- `tests/function_test.wyn`
- `tests/optional_test.wyn`
- `tests/stage1_test.wyn`
- `tests/str_find_test.wyn`
- `tests/struct_test.wyn`

## Module Organization

### io module
- `print(value)` - Print any value
- `println(value)` - Print with newline
- `print_str(s: str)` - Print string
- `print_int(n: int)` - Print integer
- `print_float(f: float)` - Print float
- `print_newline()` - Print newline
- `input(prompt: str) -> str` - Read user input
- `read_line() -> str` - Read line from stdin

### fs module
- `read_file(path: str) -> str` - Read entire file
- `write_file(path: str, content: str)` - Write file
- `append_file(path: str, content: str)` - Append to file
- `delete_file(path: str)` - Delete file
- `file_exists(path: str) -> bool` - Check if file exists
- `file_size(path: str) -> int` - Get file size
- `is_dir(path: str) -> bool` - Check if directory
- `list_dir(path: str) -> [str]` - List directory contents
- `mkdir(path: str)` - Create directory
- `rmdir(path: str)` - Remove directory

### math module
- `abs(n: int) -> int` - Absolute value
- `min(a: int, b: int) -> int` - Minimum
- `max(a: int, b: int) -> int` - Maximum
- `pow(base: int, exp: int) -> int` - Power
- `sqrt(n: int) -> int` - Square root
- `gcd(a: int, b: int) -> int` - Greatest common divisor
- `lcm(a: int, b: int) -> int` - Least common multiple
- `factorial(n: int) -> int` - Factorial
- `sin(x: float) -> float` - Sine
- `cos(x: float) -> float` - Cosine
- `tan(x: float) -> float` - Tangent
- `asin(x: float) -> float` - Arc sine
- `acos(x: float) -> float` - Arc cosine
- `atan(x: float) -> float` - Arc tangent
- `atan2(y: float, x: float) -> float` - Arc tangent 2
- `floor(x: float) -> float` - Floor
- `ceil(x: float) -> float` - Ceiling
- `round(x: float) -> float` - Round

## Migration Guide

### Before
```wyn
print("Hello")
let content: str = read_file("file.txt")
let x: float = sinf(1.0)
```

### After
```wyn
io.print("Hello")
let content: str = fs.read_file("file.txt")
let x: float = math.sin(1.0)
```

### Methods (Unchanged)
```wyn
let len: int = "hello".len()
let abs_val: int = (-5).abs()
let has: bool = [1,2,3].contains(2)
```

## Implementation Status

- [x] Create std/io.wyn module
- [x] Create std/fs.wyn module
- [x] Create std/math.wyn module
- [x] Add module.function() syntax to compiler
- [x] Update all test files
- [x] Maintain backward compatibility
- [ ] Remove legacy builtin support (future)
- [ ] Add more modules (string, collections, etc.)

## Future Work

1. Add more modules:
   - `std/string.wyn` - String manipulation
   - `std/collections.wyn` - Data structures
   - `std/time.wyn` - Time operations
   - `std/net.wyn` - Networking
   - `std/http.wyn` - HTTP client/server

2. Add import system:
   ```wyn
   import io
   import fs
   
   fn main() {
       io.print("Hello")
   }
   ```

3. Add selective imports:
   ```wyn
   from io import print, println
   from fs import read_file, write_file
   ```

4. Add module aliasing:
   ```wyn
   import io as console
   console.print("Hello")
   ```
