# Wyn Examples

Working code examples demonstrating Wyn language features.

## Quick Start Examples

### Hello World
```bash
./build/wync hello.wyn && ./a.out
```

### Basic Features
- `arithmetic.wyn` - Basic math operations
- `variables.wyn` - Variable declarations
- `conditionals.wyn` - If/else statements
- `loops.wyn` - While and for loops
- `functions.wyn` - Function definitions
- `recursion.wyn` - Recursive functions

## Core Language Features

### Variables & Types
- `variables.wyn` - let and const
- `float_test.wyn` - Float operations
- `compound_test.wyn` - Multiple types

### Control Flow
- `conditionals.wyn` - If/else
- `loops.wyn` - While/for loops
- `range_test.wyn` - Range expressions
- `break_continue_test.wyn` - Loop control ⚠️

### Functions
- `functions.wyn` - Basic functions
- `function_params.wyn` - Parameters
- `recursion.wyn` - Recursive calls
- `fibonacci.wyn` - Classic example ⚠️

### Arrays
- `array_append_demo.wyn` - Array operations
- `array_append_simple.wyn` - Simple arrays
- `array_iter_test.wyn` - Array iteration
- `slice_test.wyn` - Array slicing

### Structs & Enums
- `enum_test.wyn` - Enum declarations
- `struct_methods.wyn` - Struct with methods
- `struct_methods_full.wyn` - Full struct example
- `custom.wyn` - Custom types

### Pattern Matching
- `match_test.wyn` - Match statements
- `match_binding_test.wyn` - Pattern binding
- `result_test.wyn` - Result type patterns
- `optional_test.wyn` - Optional type patterns

## Concurrency Examples

### Spawn Blocks
- `spawn_simple.wyn` - Basic spawn
- `spawn_demo.wyn` - Multiple spawns
- `spawn_test.wyn` - Spawn with loops
- `spawn_million.wyn` - 1M concurrent tasks

### Async/Await
- `async_await_test.wyn` - Async functions
- `defer_test.wyn` - Defer statements

### Task Channels
- `task_channel_test.wyn` - Channel communication ⚠️

## Advanced Features

### Module System
- `test_simple.wyn` - Module imports
- `module_demo.wyn` - Module usage ⚠️

### Stage1 Compiler Tests
- `stage1_minimal.wyn` - Minimal program
- `stage1_simple.wyn` - Simple features
- `stage1_binop.wyn` - Binary operations
- `stage1_if.wyn` - If statements
- `stage1_while_test.wyn` - While loops
- `stage1_func.wyn` - Functions
- `stage1_call_test.wyn` - Function calls
- `stage1_expr_test.wyn` - Expressions
- `stage1_string_test.wyn` - Strings
- `stage1_all_features.wyn` - Comprehensive

### Comprehensive Tests
- `comprehensive.wyn` - All features
- `comprehensive_test.wyn` - Full test
- `complete.wyn` - Complete example ⚠️

## Stdlib Examples

### String Operations
- `string_demo.wyn` - String functions ⚠️
- `string_simple.wyn` - Basic strings ⚠️
- `string_test.wyn` - String tests ⚠️

### File I/O
- `file_test.wyn` - File operations ⚠️
- `write_file_test.wyn` - Writing files ⚠️

### Other Modules
- `compress_demo.wyn` - Compression
- `gui_test.wyn` - GUI operations
- `mobile_test.wyn` - Mobile features
- `input_test.wyn` - User input

## Special Examples

### Expressions
- `with_expr.wyn` - Expression tests
- `with_func.wyn` - Function expressions
- `with_if.wyn` - If expressions
- `with_vars.wyn` - Variable expressions
- `expr_test.wyn` - Expression evaluation ⚠️

### AST & Compiler
- `ast_test.wyn` - AST operations

### Assertions
- `assert_test.wyn` - Assert function

## Legend

- ✅ **Working** - Compiles and runs successfully
- ⚠️ **Partial** - Compiles but may have runtime issues
- ❌ **Broken** - Doesn't compile (missing features)

## Running Examples

### Single Example
```bash
./build/wync examples/loops.wyn
./a.out
```

### All Examples
```bash
make test  # Runs all examples and tests
```

### Specific Category
```bash
# Core features
for f in arithmetic variables conditionals loops functions; do
    ./build/wync examples/$f.wyn && ./a.out
done

# Concurrency
for f in spawn_simple spawn_demo async_await_test; do
    ./build/wync examples/$f.wyn && ./a.out
done
```

## Example Output

### loops.wyn
```
0
1
2
3
4
```

### spawn_demo.wyn
```
Main thread
Done spawning
Task 1
Task 2
```

### fibonacci.wyn
```
Fibonacci(10) = 55
```

## Tips

1. **Start Simple** - Begin with `hello.wyn`, `arithmetic.wyn`, `variables.wyn`
2. **Learn Concurrency** - Try `spawn_simple.wyn`, then `spawn_demo.wyn`
3. **Explore Patterns** - Check `match_test.wyn` for pattern matching
4. **Use Modules** - See `test_simple.wyn` for imports

## Contributing Examples

To add a new example:
1. Create `feature_name.wyn` in `examples/`
2. Test it: `./build/wync examples/feature_name.wyn && ./a.out`
3. Add to this README under appropriate category
4. Commit with descriptive message

## See Also

- [Getting Started](../docs/GETTING_STARTED.md) - Tutorial
- [Quick Reference](../docs/QUICK_REFERENCE.md) - Syntax reference
- [Documentation](../docs/) - Full docs
