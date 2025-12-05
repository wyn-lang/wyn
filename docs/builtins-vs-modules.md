# Builtins vs Modules in Wyn

## Philosophy

Wyn provides **two ways** to do common operations:

### 1. Builtins (Python-like simplicity)
```wyn
let content: str = read_file("test.txt")
write_file("test.txt", "hello")
print(42)
```

**When to use:**
- Quick scripts
- Simple programs
- Learning Wyn
- REPL/interactive use

### 2. Modules (Go-like organization)
```wyn
import std/io

let result: Result[str, str] = io.read_file("test.txt")
io.write_file("test.txt", "hello")
io.print(42)
```

**When to use:**
- Large projects
- Error handling needed (Result types)
- Organized code
- Library development

## Why Both?

### Builtins = Simplicity
- No imports needed
- Shorter code
- Faster to write
- Perfect for scripts

### Modules = Power
- Explicit error handling
- Namespace organization
- Type safety
- Better for production

## Module Organization

### io - Input/Output
- File reading/writing
- Console I/O
- Formatting

### fs - Filesystem
- Path manipulation (dirname, basename, join)
- Directory operations (mkdir, rmdir, list)
- File metadata (is_file, is_dir)
- File operations (copy, move, remove)

### Distinction
- **io**: Read/write data (content-focused)
- **fs**: Manage files/directories (structure-focused)

## Examples

### Quick Script (Builtins)
```wyn
fn main() {
    let data: str = read_file("input.txt")
    write_file("output.txt", data)
}
```

### Production Code (Modules)
```wyn
import std/io
import std/fs

fn main() {
    if fs.exists("input.txt") {
        let result: Result[str, str] = io.read_file("input.txt")
        if result.is_ok {
            io.write_file("output.txt", result.value)
        }
    }
}
```

## Best of Both Worlds

Wyn gives you:
- **Python's simplicity** (builtins for quick tasks)
- **Go's organization** (modules for structure)
- **Your choice** (use what fits your needs)

**One language to rule them all.**
