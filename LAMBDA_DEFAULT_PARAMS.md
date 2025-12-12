# Lambda Expressions and Default Parameters Implementation

## Status: Default Parameters COMPLETE ✅ | Lambda Syntax COMPLETE ✅

### ✅ Fully Implemented

#### 1. Default Parameters (COMPLETE)
- ✅ Parser support
- ✅ Type checker validation
- ✅ Code generation (LLVM, x86, ARM)
- ✅ String literal collection from defaults
- ✅ Call-site argument filling
- ✅ All tests passing

**Usage:**
```wyn
fn greet(name: str = "World") {
    print("Hello, " + name + "!")
}

fn add(a: int, b: int = 10) -> int {
    return a + b
}

fn main() {
    greet()           // Hello, World!
    greet("Alice")    // Hello, Alice!
    print(add(5))     // 15
    print(add(5, 20)) // 25
}
```

#### 2. Python-Style Lambda Syntax (COMPLETE)
- ✅ Parser support: `lambda x, y: x + y`
- ✅ Zero parameters: `lambda: 42`
- ✅ Multiple parameters: `lambda x, y, z: x + y + z`
- ✅ Single expression body (auto-wrapped in return)
- ✅ Lambda function generation (LLVM, x86, ARM)

**Current Limitations:**
- ⚠️ Lambdas are generated as functions but cannot be stored in variables yet
- ⚠️ First-class function support (function pointers) not implemented
- ⚠️ Cannot call lambdas through variables

**What Works:**
```wyn
// Lambda functions are generated correctly
lambda x, y: x + y  // Generates __lambda_0(i64, i64)
```

**What Doesn't Work Yet:**
```wyn
// Storing and calling through variables
const add = lambda x, y: x + y
print(add(5, 3))  // Error: tries to call 'add' as function name
```

### Implementation Details

#### Default Parameters

**Parser** (`bootstrap/stage0.c` lines 2272-2274):
- Already existed, just needed activation
- Parses `= expr` after parameter type

**Type Checker** (`bootstrap/stage0.c` lines 3670-3690):
- Counts required vs optional parameters
- Validates argument count: `required <= args <= total`
- Proper error messages

**Code Generation** (`bootstrap/stage0.c` lines 11530-11560):
- Looks up function definition
- Fills missing arguments with default values
- Evaluates default expressions at call site
- Cleans up allocated args array

**String Collection** (`bootstrap/stage0.c` lines 12635-12650):
- Collects strings from default parameter expressions
- Ensures string literals are in string table

#### Lambda Expressions

**Parser** (`bootstrap/stage0.c` lines 1675-1705):
- Python-style syntax: `lambda params: expr`
- No parentheses around parameters
- No type annotations (inferred)
- Single expression body

**Code Generation**:
- **LLVM** (`bootstrap/stage0.c` lines 11987-11999): Generates function pointer
- **x86** (`bootstrap/stage0.c` line 6559): Stores lambda, returns pointer
- **ARM** (`bootstrap/stage0.c` line 8298): Stores lambda, returns pointer

**Lambda Emission** (`bootstrap/stage0.c` lines 12895-12930):
- Generates `__lambda_N` functions
- Parameters become function parameters
- Body statements emitted
- Returns i64 (for now)

### Testing

**Test Files:**
- `tests/default_params_test.wyn` - ✅ All tests passing
- `examples/default_simple.wyn` - ✅ Working
- `examples/default_params_test.wyn` - ✅ Working
- `examples/lambda_python_test.wyn` - ⚠️ Syntax works, calls don't

**Test Results:**
```
Tests: 28 passed, 0 failed
✅ Test suite complete!
```

### What's Next for Lambdas

To make lambdas fully functional (first-class functions):

1. **Function Pointer Type System**:
   - Add `TYPE_FUNCTION_PTR` to type system
   - Track function signatures
   - Type check function pointer calls

2. **Variable Storage**:
   - Store function pointers as i64
   - Cast back to function type when calling
   - Handle in EXPR_CALL when func is EXPR_IDENT

3. **Indirect Calls**:
   ```llvm
   %fn_ptr = load i64, i64* %var
   %fn = inttoptr i64 %fn_ptr to i64 (i64, i64)*
   %result = call i64 %fn(i64 %arg1, i64 %arg2)
   ```

4. **Closure Support** (future):
   - Capture analysis
   - Closure struct allocation
   - Environment passing

### Estimated Effort for Full Lambda Support

- **Function pointers**: 4-6 hours
- **Closure support**: 8-12 hours
- **Total**: 12-18 hours

### References

- Parser: `bootstrap/stage0.c` lines 1675-1705, 2272-2274
- Type checker: `bootstrap/stage0.c` lines 3670-3690
- LLVM codegen: `bootstrap/stage0.c` lines 11530-11560, 11987-11999, 12895-12930
- x86 codegen: `bootstrap/stage0.c` line 6559
- ARM codegen: `bootstrap/stage0.c` line 8298
