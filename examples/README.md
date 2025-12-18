# Wyn Examples

Working code examples demonstrating Wyn language features.

## Quick Start

```bash
./build/wyn run hello.wyn
```

## New Features (v0.3.0)

### Method Syntax
- `method_chaining_demo.wyn` - Method calls on values
- `string_operations.wyn` - String methods (.upper(), .lower(), .trim())
- `array_operations.wyn` - Array methods (.len(), .get())

### Extension Methods
- `extension_methods.wyn` - Add custom methods to types (Python/JavaScript style)
  ```wyn
  fn int.squared(self) -> int { return self * self }
  5.squared()  // 25
  ```

### File System
- `fs_demo.wyn` - File operations with fs module

## Core Language

### Variables & Types
- `variables.wyn` - let and const
- `arithmetic.wyn` - Basic math
- `conditionals.wyn` - If/else
- `loops.wyn` - While/for loops
- `functions.wyn` - Function definitions
- `recursion.wyn` - Recursive functions
- `fibonacci.wyn` - Classic example

### Advanced Features
- `closures_demo.wyn` - Closures and variable capture
- `destructuring_demo.wyn` - Pattern destructuring
- `traits_demo.wyn` - Trait-like polymorphism
- `async_await_demo.wyn` - Async patterns
- `generics_test.wyn` - Generic functions

### Error Handling
- `error_handling_demo.wyn` - Result and Option types
- `result_option_demo.wyn` - Pattern matching with Result/Option
- `advanced_error_handling.wyn` - Comprehensive error handling

### Concurrency
- `spawn_demo.wyn` - Spawn blocks
- `spawn_simple.wyn` - Simple concurrency
- `spawn_million.wyn` - 1M concurrent tasks

### Arrays & Collections
- `array_operations.wyn` - Array utilities
- `array_append_demo.wyn` - Array manipulation
- `comprehension.wyn` - List comprehensions

### Strings
- `string_operations.wyn` - String methods
- `string_interpolation_demo.wyn` - String formatting
- `multiline_string_demo.wyn` - Multiline strings

### Pattern Matching
- `match_test.wyn` - Match expressions
- `match_binding_test.wyn` - Variable binding in patterns
- `match_enum_test.wyn` - Enum matching

### DevOps & Automation
- `devops_automation.wyn` - DevOps tasks
- `shell_replacement_demo.wyn` - Shell script replacement
- `command_builder_demo.wyn` - Command building
- `command_chaining.wyn` - Command chaining
- `file_finder.wyn` - File search utility

### Web & Network
- `http_server.wyn` - HTTP server
- `infrastructure_demo.wyn` - Infrastructure code
- `extended_infra_demo.wyn` - Extended infrastructure

### Testing
- `testing_demo.wyn` - Testing framework

## Production Examples

See individual files for production-ready code examples.

## Running Examples

```bash
# Run directly
./build/wyn run examples/hello.wyn

# Compile first
./build/wyn compile examples/hello.wyn
./a.out
```

## Total Examples

100+ working examples covering all language features.
