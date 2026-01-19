# Wyn Build System Guide

Complete guide to building Wyn projects.

## Quick Start

### Single File Compilation

Compile a single Wyn file:
```bash
./wyn myfile.wyn
```

This generates `myfile.wyn.out` which you can run:
```bash
./myfile.wyn.out
```

### Multi-File Projects

Build a project with multiple files:
```bash
./wyn build myproject/
```

This generates `myproject/main` executable:
```bash
./myproject/main
```

## Build Commands

### wyn [file]
Compiles a single Wyn file.

**Example:**
```bash
./wyn hello.wyn
./hello.wyn.out
```

### wyn build [directory]
Builds a multi-file project.

**Example:**
```bash
./wyn build src/
./src/main
```

**How it works:**
1. Finds all `.wyn` files in directory
2. Combines them into single compilation unit
3. Generates executable named `main`

## Project Structure

### Single File Project
```
myproject/
  main.wyn
```

### Multi-File Project
```
myproject/
  main.wyn
  utils.wyn
  math.wyn
```

### With Modules
```
myproject/
  main.wyn
  math.wyn      # export fn add() ...
  utils.wyn     # export fn helper() ...
```

## Compilation Process

### Step 1: Parsing
Wyn parses all source files and builds an AST.

### Step 2: Type Checking
The type checker verifies all types are correct.

### Step 3: Code Generation
Generates C code from the AST.

### Step 4: C Compilation
Compiles generated C code with gcc.

## Optimization Levels

### Default (O0)
No optimization, fastest compilation.
```bash
./wyn myfile.wyn
```

### Optimized (O2)
Full optimization, slower compilation.
```bash
./wyn myfile.wyn -O2
```

## Generated Files

### .wyn.c Files
Generated C code (intermediate).

### .wyn.out Files
Compiled executables.

### Cleaning Up
Remove generated files:
```bash
rm *.wyn.c *.wyn.out
```

## Build Errors

### Compilation Errors
Wyn shows errors with source context:
```
Error: Expected ')' after parameters
  --> main.wyn:5:1
   |
 5 | fn add(a: int, b: int -> int {
   | ^
```

### Linker Errors
If C compilation fails, check:
1. All required source files included
2. No duplicate function definitions
3. All dependencies available

## Advanced Usage

### Custom Output Name
Currently not supported. Use:
```bash
./wyn myfile.wyn
mv myfile.wyn.out myapp
```

### Include Paths
The compiler automatically includes:
- Current directory
- `src/` directory

### Dependencies
All Wyn runtime dependencies are automatically linked:
- Standard library
- Runtime support (ARC, async, etc.)
- System libraries (pthread, math)

## Troubleshooting

### "No .wyn files found"
Ensure directory contains `.wyn` files.

### "Compilation failed"
Check syntax errors in source files.

### "Undefined symbol"
Ensure all functions are defined or imported.

## Best Practices

1. **Organize by feature** - Group related files
2. **Use modules** - Export public APIs
3. **Keep main.wyn simple** - Entry point only
4. **Test incrementally** - Build after each change

## Examples

### Example 1: Simple Project
```bash
# Create project
mkdir myapp
echo 'fn main() -> int { return 0; }' > myapp/main.wyn

# Build
./wyn build myapp/

# Run
./myapp/main
```

### Example 2: With Modules
```bash
# Create project
mkdir calc
cat > calc/math.wyn << 'EOF'
export fn add(a: int, b: int) -> int {
    return a + b;
}
EOF

cat > calc/main.wyn << 'EOF'
import math;
fn main() -> int {
    return math::add(1, 2);
}
EOF

# Build
./wyn build calc/

# Run
./calc/main
echo "Exit: $?"  # Should be 3
```

## See Also

- [Language Guide](LANGUAGE_GUIDE.md)
- [Module Guide](MODULE_GUIDE.md)
- [Standard Library](STDLIB_REFERENCE.md)
