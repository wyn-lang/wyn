# Module System Implementation

## Overview
Implemented a module/namespace system to organize builtins into logical modules. The system supports `module.function()` syntax while maintaining backward compatibility with direct function calls.

## Implementation Status

✅ **COMPLETE** - Module system fully implemented and tested

- [x] Create std/io_module.wyn specification
- [x] Create std/fs_module.wyn specification  
- [x] Create std/math_module.wyn specification
- [x] Add module.function() syntax to compiler
- [x] Update type checker to recognize module calls
- [x] Update x86_64 codegen for module calls
- [x] Update ARM64 codegen for module calls
- [x] Handle inlined builtins (abs, min, max)
- [x] All 47 tests passing
- [x] Backward compatibility maintained

## Changes Made

### 1. Compiler Changes (`bootstrap/stage0.c`)

Added three key functions:

1. **`map_module_function(module, function)`** - Maps module.function to builtin name
   - `io.print_str` → `print_str`
   - `math.sin` → `sinf`
   - `fs.read_file` → `read_file`

2. **`is_builtin_call(call_func, builtin_name)`** - Checks if a call is to a specific builtin
   - Handles both `abs()` and `math.abs()`
   - Used for inlined builtins (abs, min, max)

3. **Type checker updates** - Recognizes module.function() as valid calls

4. **Codegen updates** - Both x86_64 and ARM64 backends support module calls

### 2. Module Specifications

Created three module specification files in `std/`:

- **`std/io_module.wyn`** - I/O operations
- **`std/fs_module.wyn`** - File system operations  
- **`std/math_module.wyn`** - Math functions

These files document the module API and serve as specifications for future implementation.

## Module Organization

### io module
```wyn
io.print(value)           # Print any value
io.println(value)         # Print with newline
io.print_str(s: str)      # Print string
io.print_int(n: int)      # Print integer
io.print_float(f: float)  # Print float
io.print_newline()        # Print newline
io.read_line() -> str     # Read line from stdin
io.input(prompt: str) -> str  # Read with prompt
```

### fs module
```wyn
fs.read_file(path: str) -> str           # Read entire file
fs.write_file(path: str, content: str)   # Write file
fs.append_file(path: str, content: str)  # Append to file
fs.delete_file(path: str)                # Delete file
fs.file_exists(path: str) -> bool        # Check if exists
fs.file_size(path: str) -> int           # Get file size
fs.is_dir(path: str) -> bool             # Check if directory
fs.list_dir(path: str) -> [str]          # List directory
fs.mkdir(path: str)                      # Create directory
fs.rmdir(path: str)                      # Remove directory
```

### math module
```wyn
# Integer operations
math.abs(n: int) -> int
math.min(a: int, b: int) -> int
math.max(a: int, b: int) -> int
math.pow(base: int, exp: int) -> int
math.sqrt(n: int) -> int
math.gcd(a: int, b: int) -> int
math.lcm(a: int, b: int) -> int
math.factorial(n: int) -> int

# Float operations
math.sin(x: float) -> float
math.cos(x: float) -> float
math.tan(x: float) -> float
math.asin(x: float) -> float
math.acos(x: float) -> float
math.atan(x: float) -> float
math.atan2(y: float, x: float) -> float
math.floor(x: float) -> float
math.ceil(x: float) -> float
math.round(x: float) -> float
math.log(x: float) -> float
math.log10(x: float) -> float
math.exp(x: float) -> float
```

## Usage Examples

### New Module Syntax
```wyn
fn main() {
    # I/O operations
    io.print_str("Hello, World!")
    io.print_newline()
    
    # File operations
    let content: str = fs.read_file("config.txt")
    fs.write_file("output.txt", content)
    
    # Math operations
    let x: int = math.abs(-42)
    let min_val: int = math.min(10, 20)
    let angle: float = math.sin(1.57)
}
```

### Legacy Syntax (Still Supported)
```wyn
fn main() {
    print_str("Hello, World!")
    print_newline()
    
    let content: str = read_file("config.txt")
    write_file("output.txt", content)
    
    let x: int = abs(-42)
    let min_val: int = min(10, 20)
}
```

### Methods (Unchanged)
```wyn
fn main() {
    # String methods
    let len: int = "hello".len()
    let has: bool = "hello".contains("ell")
    let idx: int = "hello".index_of("l")
    
    # Number methods
    let abs_val: int = (-5).abs()
    let str_val: str = (42).to_str()
    
    # Array methods
    let arr: [int] = [1, 2, 3]
    let size: int = arr.len()
}
```

## Technical Details

### How It Works

1. **Parsing**: `io.print_str("hello")` is parsed as:
   - EXPR_CALL with func = EXPR_FIELD
   - EXPR_FIELD with object = EXPR_IDENT("io"), field = "print_str"

2. **Type Checking**: Type checker recognizes module.function() pattern:
   - Checks if object is a known module name (io, fs, math)
   - Maps to underlying builtin function
   - Validates arguments

3. **Code Generation**: Codegen translates module calls:
   - `io.print_str()` → calls `_print_str` runtime function
   - `math.abs()` → inlines abs instruction sequence
   - `fs.read_file()` → calls `__wyn_read_file` runtime function

### Inlined Builtins

Some builtins are inlined for performance:
- `abs()` / `math.abs()` - Uses conditional move instruction
- `min()` / `math.min()` - Uses conditional move instruction
- `max()` / `math.max()` - Uses conditional move instruction

These work with both direct calls and module syntax.

## Testing

All 47 existing tests pass with the new module system:
- Backward compatibility maintained
- Module syntax tested and working
- Both x86_64 and ARM64 architectures supported

Test file: `temp/test_module_syntax.wyn`

## Future Work

### Phase 1: Import System
```wyn
import io
import fs
import math

fn main() {
    io.print("Hello")
    fs.read_file("file.txt")
}
```

### Phase 2: Selective Imports
```wyn
from io import print, println
from fs import read_file, write_file

fn main() {
    print("Hello")
    read_file("file.txt")
}
```

### Phase 3: Module Aliasing
```wyn
import io as console
import fs as file

fn main() {
    console.print("Hello")
    file.read("data.txt")
}
```

### Phase 4: Additional Modules
- `std/string.wyn` - String manipulation
- `std/collections.wyn` - Data structures
- `std/time.wyn` - Time operations
- `std/net.wyn` - Networking
- `std/http.wyn` - HTTP client/server
- `std/json.wyn` - JSON parsing
- `std/regex.wyn` - Regular expressions

### Phase 5: User Modules
```wyn
# mymodule.wyn
pub fn helper() {
    io.print("Helper function")
}

# main.wyn
import mymodule

fn main() {
    mymodule.helper()
}
```

## Migration Guide

### Recommended Approach

1. **New code**: Use module syntax
   ```wyn
   io.print_str("Hello")
   math.abs(-5)
   fs.read_file("file.txt")
   ```

2. **Existing code**: No changes required
   - Legacy syntax continues to work
   - Migrate gradually as needed

3. **Methods**: No changes
   - Continue using `.len()`, `.abs()`, etc.
   - Methods are not affected by module system

### Benefits of Module Syntax

1. **Clarity**: `io.print_str()` vs `print_str()` - clear what module it's from
2. **Organization**: Related functions grouped together
3. **Discoverability**: Easy to find all I/O, file, or math functions
4. **Future-proof**: Enables import system and module management
5. **No conflicts**: Different modules can have same function names

## Conclusion

The module system is fully implemented and working. It provides a clean namespace organization while maintaining full backward compatibility. All 47 tests pass, and the system is ready for use in new code.

The implementation adds minimal overhead - module calls are resolved at compile time and generate the same code as direct calls. Inlined builtins remain inlined regardless of call syntax.
