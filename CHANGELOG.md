# Changelog

## [0.1.0] - 2025-11-30

### Added
- Stage 0 bootstrap compiler in C
- ARM64 and x86_64 code generation for macOS
- Core language features:
  - Functions with parameters and return values
  - Recursion
  - Variables (`let`, `let mut`)
  - All arithmetic operators (`+`, `-`, `*`, `/`, `%`, `**`)
  - Bitwise operators (`|`, `&`, `^`, `~`, `<<`, `>>`)
  - Comparison and logical operators
  - Control flow (`if`/`else`, `while`, `for`, `match`)
  - Range syntax (`0..10`, `0..=10`)
  - `break` and `continue`
- Type system:
  - Primitive types: `int`, `float`, `str`, `bool`
  - Arrays with indexing and slicing
  - Structs with methods
  - Enums with integer values
  - Optional types (`?T`, `some`, `none`)
  - Result types (`Result[T, E]`, `ok`, `err`)
  - Generics for structs and functions
- Built-in functions:
  - `print()`, `print_str()` for output
  - `assert()` for runtime assertions
  - `exit()` for program termination
  - `abs()`, `min()`, `max()` math functions
  - `len()` for arrays and strings
  - `int_to_str()`, `str_to_int()` conversions
  - `ord()`, `chr()` character operations
  - `read_file()` for file I/O
- Method syntax on primitives (`.len()`, `.abs()`, `.to_str()`, `.to_int()`)
- Import system for modules
- Standard library modules:
  - math (sqrt, floor, ceil, round, clamp, gcd, lcm, factorial, etc.)
  - string (trim, contains, starts_with, ends_with, repeat, reverse, pad, etc.)
  - collections (array utilities, Stack, Queue)
  - io (formatting, file operations)
  - os (environment, arguments)
  - time (duration, date utilities)
  - fs (path manipulation)
  - net, http, json, crypto, regex (partial)
  - log (logging levels)
  - test (assertions)
- Test suite with 31 passing tests
- Makefile build system with install target
- Documentation:
  - README.md with quick start
  - GETTING_STARTED.md tutorial
  - SPEC.md language specification
  - std/README.md library documentation

### Known Limitations
- String comparison after concatenation may fail (pointer comparison)
- String slicing on variables has issues
- No garbage collection (stack allocation only)
- Limited to 256 local variables per function
- No closures or first-class functions yet
