# Wyn Examples

Quick start guide for running Wyn examples with the LLVM backend.

## Quick Start

```bash
# 1. Build the compiler (if not already built)
make

# 2. Run an example
./build/wync examples/hello.wyn
./a.out

# Or specify output file
./build/wync examples/hello.wyn -o hello
./hello
```

## Recommended Examples for Learning

Start with these examples in order:

### 1. Hello World
```bash
./build/wync examples/hello.wyn -o hello && ./hello
```
Learn: Basic program structure, print statements

### 2. Variables
```bash
./build/wync examples/variables.wyn -o variables && ./variables
```
Learn: Variable declaration, types, assignment

### 3. Arithmetic
```bash
./build/wync examples/arithmetic.wyn -o arithmetic && ./arithmetic
```
Learn: Math operations (+, -, *, /)

### 4. Functions
```bash
./build/wync examples/functions.wyn -o functions && ./functions
```
Learn: Function definitions, parameters, return values

### 5. Conditionals
```bash
./build/wync examples/conditionals.wyn -o conditionals && ./conditionals
```
Learn: If statements, comparisons

### 6. Function Parameters
```bash
./build/wync examples/function_params.wyn -o function_params && ./function_params
```
Learn: Complex function calls, nested calls

## Working Examples

These examples compile and run successfully:

| Example | Features | Output |
|---------|----------|--------|
| `hello.wyn` | Print, strings | "Hello, World!" |
| `variables.wyn` | Variables, types | Numbers |
| `arithmetic.wyn` | Math operations | Calculations |
| `functions.wyn` | Functions, params | Function results |
| `function_params.wyn` | Nested calls | Computed value |
| `conditionals.wyn` | If statements | Conditional output |
| `expr_test.wyn` | While loops, imports | Loop iterations |
| `float_test.wyn` | Float types | Test message |

## Examples with Known Issues

These examples have LLVM backend issues:

| Example | Issue | Status |
|---------|-------|--------|
| `fibonacci.wyn` | Recursion + loops | ❌ LLVM IR error |
| `fizzbuzz.wyn` | Missing println() | ❌ Type error |
| `loops.wyn` | For/while loops | ❌ LLVM IR error |
| `recursion.wyn` | Recursive calls | ❌ LLVM IR error |
| `llvm_demo.wyn` | Arrays + loops | ❌ LLVM IR error |
| `simple.wyn` | Complex expressions | ❌ LLVM IR error |
| `complete.wyn` | Structs, advanced | ❌ Not tested |

## Compiler Options

```bash
# Basic compilation
./build/wync program.wyn

# Specify output file
./build/wync program.wyn -o myprogram

# Use native ARM64 backend (legacy)
./build/wync --backend native program.wyn

# Cross-compile (LLVM backend)
./build/wync --target x86_64 --target-os linux program.wyn
```

## Current LLVM Backend Status

**Working:**
- ✅ Variables and constants
- ✅ Arithmetic operations
- ✅ Function definitions and calls
- ✅ Parameters and return values
- ✅ If statements
- ✅ String literals
- ✅ Print statements

**Issues:**
- ⚠️ For/while loops (LLVM IR numbering)
- ⚠️ Recursion (register numbering)
- ⚠️ Arrays (needs work)
- ❌ println() not implemented
- ❌ Else/else-if chains
- ❌ Float arithmetic

## Writing Your Own Examples

### Template for a working example:

```wyn
fn main() {
    print("Hello")
    
    let x: int = 42
    print(x)
    
    let result: int = add(10, 20)
    print(result)
}

fn add(a: int, b: int) -> int {
    return a + b
}
```

### What to avoid (for now):

```wyn
# Don't use for loops yet
for i in 0..10 {  # ❌ LLVM IR error
    print(i)
}

# Don't use recursion yet
fn factorial(n: int) -> int {  # ❌ LLVM IR error
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}

# Don't use println
println("text")  # ❌ Not implemented

# Don't use arrays yet
let arr: [int] = [1, 2, 3]  # ⚠️ May have issues
print(arr[0])
```

## Testing Your Example

```bash
# Try to compile
./build/wync examples/myexample.wyn -o /tmp/test

# If successful, run it
/tmp/test

# If it fails, check the error
# LLVM IR errors usually mean register numbering issues
```

## Documentation

For detailed information about working examples, see:
- [docs/WORKING_EXAMPLES.md](../docs/WORKING_EXAMPLES.md) - Full list with expected output
- [docs/LLVM_COMPLETE.md](../docs/LLVM_COMPLETE.md) - LLVM backend status
- [docs/GETTING_STARTED.md](../docs/GETTING_STARTED.md) - Language guide

## Contributing Examples

When adding new examples:
1. Test with LLVM backend first
2. Use simple, clear code
3. Add comments explaining features
4. Follow naming: `feature_name.wyn`
5. Update this README if it works

## Getting Help

If an example doesn't work:
1. Check [docs/WORKING_EXAMPLES.md](../docs/WORKING_EXAMPLES.md) for known issues
2. Look at the LLVM IR: `cat /tmp/wyn_*.ll`
3. Try simplifying the code
4. Check if you're using unsupported features
