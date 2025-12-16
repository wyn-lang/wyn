# Working Examples with LLVM Backend

This document lists all verified working examples with the LLVM backend.

## Status: Basic Features Working ✅

The LLVM backend currently supports:
- ✅ Variables and constants
- ✅ Basic arithmetic (+, -, *, /)
- ✅ Function definitions and calls
- ✅ Function parameters and return values
- ✅ Conditionals (if statements)
- ✅ String literals
- ✅ Print statements
- ⚠️ Loops (for/while) - LLVM IR generation issues
- ⚠️ Recursion - LLVM IR numbering issues
- ⚠️ Arrays - needs testing

## Working Examples

### 1. hello.wyn - Hello World
**Location:** `examples/hello.wyn`

**What it demonstrates:**
- Basic program structure
- Print statement
- String literals

**Code:**
```wyn
fn main() {
    print("Hello, World!")
}
```

**Expected output:**
```
Hello, World!
```

**Usage:**
```bash
./build/wync examples/hello.wyn -o hello
./hello
```

---

### 2. variables.wyn - Variable Declaration
**Location:** `examples/variables.wyn`

**What it demonstrates:**
- Variable declaration with `let`
- Integer type annotations
- Variable assignment
- Multiple variables

**Code:**
```wyn
fn main() {
    let x: int = 42
    let y: int = 10
    let z: int = x
    
    print(x)
    print(y)
    print(z)
}
```

**Expected output:**
```
421042
```

**Usage:**
```bash
./build/wync examples/variables.wyn -o variables
./variables
```

---

### 3. arithmetic.wyn - Arithmetic Operations
**Location:** `examples/arithmetic.wyn`

**What it demonstrates:**
- Addition, subtraction, multiplication, division
- Expression evaluation
- Printing results

**Code:**
```wyn
fn main() {
    let x: int = 10
    let y: int = 5
    
    print(x + y)
    print(x - y)
    print(x * y)
    print(x / y)
}
```

**Expected output:**
```
155502
```

**Usage:**
```bash
./build/wync examples/arithmetic.wyn -o arithmetic
./arithmetic
```

---

### 4. functions.wyn - Function Definitions
**Location:** `examples/functions.wyn`

**What it demonstrates:**
- Function definitions
- Function calls
- Parameters and return values
- Void functions (no return)

**Code:**
```wyn
fn greet() {
    print("Hello from function")
}

fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    greet()
    let result: int = add(10, 20)
    print(result)
}
```

**Expected output:**
```
Hello from function30
```

**Usage:**
```bash
./build/wync examples/functions.wyn -o functions
./functions
```

---

### 5. function_params.wyn - Complex Function Calls
**Location:** `examples/function_params.wyn`

**What it demonstrates:**
- Multiple function definitions
- Nested function calls
- Parameter passing
- Return value usage

**Code:**
```wyn
fn add(x: int, y: int) -> int {
    return x + y
}

fn multiply(a: int, b: int) -> int {
    return a * b
}

fn compute(x: int, y: int) -> int {
    let sum: int = add(x, y)
    let product: int = multiply(x, y)
    return add(sum, product)
}

fn main() {
    let result: int = compute(3, 4)
    print(result)
}
```

**Expected output:**
```
19
```
*(3+4=7, 3*4=12, 7+12=19)*

**Usage:**
```bash
./build/wync examples/function_params.wyn -o function_params
./function_params
```

---

### 6. conditionals.wyn - If Statements
**Location:** `examples/conditionals.wyn`

**What it demonstrates:**
- If statements
- Comparison operators (>, <)
- Multiple conditionals
- Control flow

**Code:**
```wyn
fn main() {
    let x: int = 15
    
    if x > 10 {
        print("x is greater than 10")
    }
    
    if x < 20 {
        print("x is less than 20")
    }
    
    let y: int = 5
    if y > 10 {
        print("y is large")
    }
}
```

**Expected output:**
```
x is greater than 10x is less than 20
```

**Usage:**
```bash
./build/wync examples/conditionals.wyn -o conditionals
./conditionals
```

---

### 7. expr_test.wyn - Expressions with Imports
**Location:** `examples/expr_test.wyn`

**What it demonstrates:**
- Import statements
- While loops (basic)
- Variable mutation
- Comments

**Code:**
```wyn
import io

fn main() {
    let x: int = 5
    let y: int = 10
    const max: int = 15
    
    print("Testing expressions")
    
    while x < max {
        print("Loop iteration")
        x = x + 1
    }
    
    print("Done")
}
```

**Expected output:**
```
Testing expressionsLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationLoop iterationDone
```

**Usage:**
```bash
./build/wync examples/expr_test.wyn -o expr_test
./expr_test
```

---

### 8. float_test.wyn - Float Type Support
**Location:** `examples/float_test.wyn`

**What it demonstrates:**
- Float type declaration
- Const keyword
- Float storage (printing not yet implemented)

**Code:**
```wyn
fn main() {
    const x: float = 3.14
    const y: float = 2.0
    
    print(42)
    print("Float test passed!")
}
```

**Expected output:**
```
42Float test passed!
```

**Usage:**
```bash
./build/wync examples/float_test.wyn -o float_test
./float_test
```

---

## Known Issues

### 1. LLVM IR Register Numbering
**Symptoms:** Compilation fails with errors like:
```
error: instruction expected to be numbered '%N' or greater
```

**Affected features:**
- For loops
- While loops (in some cases)
- Recursive functions
- Array operations

**Examples that fail:**
- `loops.wyn` - for/while loops
- `recursion.wyn` - factorial recursion
- `llvm_demo.wyn` - comprehensive demo
- `fibonacci.wyn` - recursive fibonacci

**Root cause:** LLVM IR generator has issues with SSA register numbering in complex control flow.

### 2. Missing Features
- `println()` function not implemented (use `print()` instead)
- Float arithmetic not implemented
- Array indexing has issues
- Else/else-if not fully working
- Break/continue not implemented

## Compilation Tips

### Basic compilation:
```bash
./build/wync program.wyn
./a.out
```

### Specify output file:
```bash
./build/wync program.wyn -o myprogram
./myprogram
```

### View LLVM IR:
```bash
./build/wync program.wyn
# IR is written to /tmp/wyn_*.ll
cat /tmp/wyn_*.ll
```

## What Works vs What Doesn't

### ✅ Works Well
- Simple programs with print statements
- Variable declarations and assignments
- Function definitions with parameters
- Function calls and return values
- Basic arithmetic expressions
- Simple if statements
- String literals

### ⚠️ Partially Works
- While loops (simple cases work, complex ones fail)
- Imports (parsed but not fully functional)
- Float types (storage works, arithmetic doesn't)

### ❌ Doesn't Work Yet
- For loops (LLVM IR issues)
- Recursive functions (register numbering)
- Arrays (indexing issues)
- Else/else-if chains
- Break/continue statements
- Pattern matching
- Structs
- Enums

## Next Steps

To improve LLVM backend:
1. Fix SSA register numbering in control flow
2. Implement proper loop code generation
3. Add array support
4. Implement else/else-if
5. Add float arithmetic operations
6. Implement println() builtin

## Testing New Examples

To test if an example works:

```bash
# Try to compile
./build/wync examples/yourfile.wyn -o /tmp/test

# If successful, run it
/tmp/test

# If it fails, check the LLVM IR
cat /tmp/wyn_*.ll
```

Look for LLVM compilation errors - they usually indicate register numbering issues.
