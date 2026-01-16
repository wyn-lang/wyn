# Wyn Programming Language

A modern systems programming language that compiles to C.

## Features

- **Modern Syntax** - Clean, expressive syntax inspired by Rust and Go
- **Generics** - Full generic programming support with monomorphization
- **Async/Await** - First-class asynchronous programming
- **Module System** - Organize code with imports and exports
- **Memory Safety** - Automatic Reference Counting (ARC)
- **Concurrency** - Built-in threading support
- **Rich Standard Library** - HashMap, HashSet, JSON, File I/O, and more

## Quick Start

### Installation

```bash
cd wyn
make wyn
```

### Hello World

```wyn
fn main() -> int {
    print_str("Hello, World!");
    return 0;
}
```

Compile and run:
```bash
./wyn hello.wyn
./hello.wyn.out
```

## Examples

See the `examples/` directory for more:

- `01_hello_world.wyn` - Basic hello world
- `02_functions.wyn` - Function definitions
- `03_generics.wyn` - Generic programming
- `04_async_await.wyn` - Asynchronous code
- `05_file_io.wyn` - File operations
- `06_hashmap.wyn` - Hash maps
- `07_modules/` - Module system
- `08_hashset.wyn` - Hash sets
- `09_json.wyn` - JSON parsing
- `10_structs.wyn` - Structures

## Documentation

- [Language Guide](LANGUAGE_GUIDE.md) - Complete language reference
- [Standard Library](STDLIB_REFERENCE.md) - All stdlib functions
- [Build System](BUILD_GUIDE.md) - Building projects
- [Module System](MODULE_GUIDE.md) - Using modules

## Building Projects

Single file:
```bash
./wyn myfile.wyn
./myfile.wyn.out
```

Multi-file project:
```bash
./wyn build myproject/
./myproject/main
```

## Language Features

### Functions
```wyn
fn add(a: int, b: int) -> int {
    return a + b;
}
```

### Generics
```wyn
fn identity<T>(x: T) -> T {
    return x;
}
```

### Async/Await
```wyn
async fn fetch() -> int {
    return 42;
}

fn main() -> int {
    let future = fetch();
    let result = await future;
    return result;
}
```

### Modules
```wyn
// math.wyn
export fn add(a: int, b: int) -> int {
    return a + b;
}

// main.wyn
import math;
fn main() -> int {
    return math::add(1, 2);
}
```

### Structs
```wyn
struct Point {
    x: int,
    y: int
}

fn main() -> int {
    let p = Point { x: 10, y: 20 };
    return p.x + p.y;
}
```

## Standard Library

- **Collections**: HashMap, HashSet
- **I/O**: File operations, console I/O
- **JSON**: Parsing and manipulation
- **Strings**: String operations
- **Math**: Mathematical functions
- **Concurrency**: Threading primitives
- **Async**: Future-based async operations

## Testing

Run the test suite:
```bash
bash /tmp/run_all_tests.sh
```

## License

MIT License

## Contributing

Contributions welcome! Please see CONTRIBUTING.md for guidelines.
