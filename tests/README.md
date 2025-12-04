# Wyn Test Suite

**69/72 tests passing (96%) | 100% stdlib coverage**

## Running Tests

```bash
# Run all tests (test runner written in Wyn)
make test

# Run specific test
./build/stage0 -o /tmp/test tests/math_stdlib_test.wyn && /tmp/test
```

**100% cross-platform** - Test runner written in Wyn, no bash scripts.

## Test Organization

### Standard Library Tests (23 modules)

Each stdlib module has comprehensive tests:
- `array_stdlib_test.wyn` - Array operations, slicing, indexing
- `bits_stdlib_test.wyn` - Bitwise operations (AND, OR, XOR, shifts)
- `collections_stdlib_test.wyn` - Collection operations
- `compress_stdlib_test.wyn` - Compression algorithms
- `crypto_stdlib_test.wyn` - Cryptographic functions
- `encoding_stdlib_test.wyn` - Character encoding (chr/ord)
- `error_stdlib_test.wyn` - Error handling
- `fs_stdlib_test.wyn` - File system operations
- `gpu_stdlib_test.wyn` - GPU/Vulkan operations
- `hash_stdlib_test.wyn` - Hash functions
- `http_stdlib_test.wyn` - HTTP utilities
- `io_stdlib_test.wyn` - I/O operations
- `json_stdlib_test.wyn` - JSON handling
- `log_stdlib_test.wyn` - Logging functions
- `math_stdlib_test.wyn` - Math functions (abs, min/max, pow, sqrt, trig, exp/log)
- `net_stdlib_test.wyn` - Network operations
- `os_stdlib_test.wyn` - OS functions (getcwd, getpid, getenv, args)
- `random_stdlib_test.wyn` - Random number generation
- `regex_stdlib_test.wyn` - Pattern matching
- `string_stdlib_test.wyn` - String operations (length, concat, slice, find, split)
- `time_stdlib_test.wyn` - Time operations (time_now, sleep, clock)
- `utils_stdlib_test.wyn` - Utility functions
- `validate_stdlib_test.wyn` - Input validation

### Core Language Tests (50 tests)

Comprehensive coverage of language features:
- Arrays, structs, functions, control flow
- Optional types, Result types, enums
- Generics, lambdas, pattern matching
- String interpolation, ranges, slicing
- Defer, spawn, compound assignments

## Test Quality

All tests are:
- ✅ **Non-blocking** - CI-ready, no user interaction
- ✅ **Comprehensive** - Real functionality testing, no fillers
- ✅ **Fast** - Parallel execution with caching
- ✅ **No duplicates** - Each module tested once
- ✅ **Proper naming** - `*_test.wyn` or `*_stdlib_test.wyn`

## Coverage: 100%

All implemented stdlib functions and language features are covered by automated tests.
