# Stage 1 Compiler Progress

## Goal
Make Stage 1 compiler capable of compiling its own source code (self-hosting).

## Completed Improvements ✅

### 1. Dynamic Loop Bound Parsing
**Before:** Hardcoded `cmp x0, #3` in assembly
**After:** Parses actual value from `while x < N` in source code

Example:
```wyn
while x < 100 {  // Compiler now parses 100 from source
    print("Hello")
}
```

### 2. Dynamic Variable Initialization
**Before:** Hardcoded `mov x0, #0` for all variables
**After:** Parses actual initial value from `let x = N`

Example:
```wyn
let x: int = 50  // Compiler now parses 50 from source
```

### 3. Const Declaration Support
**After:** Parser recognizes both `let` and `const` declarations

Example:
```wyn
const max: int = 100  // Now supported
let x: int = 0
```

## Test Results
All 4 tests passing:
- ✅ Basic loop (0 to 3)
- ✅ Large bound (0 to 100) 
- ✅ Non-zero init (50 to 55)
- ✅ Const declaration (7 to 9)

Run tests: `./test_stage1_improvements.sh`

## Next Steps for Self-Hosting

### Critical Features Needed:
1. **Function Parameters** - Parse and pass arguments
2. **Return Statements** - Parse `return expr` and generate code
3. **Multiple Variables** - Track multiple vars with different stack offsets
4. **Variable References** - Support `while x < max` (not just literals)
5. **Assignment Parsing** - Parse `x = x + 1` dynamically (currently hardcoded)
6. **String Variables** - Handle string types, not just int
7. **Function Calls** - Parse and generate calls to other functions
8. **If Statements** - Parse conditions and branches

### Current Limitations:
- Only one variable supported (hardcoded at `[x29, #16]`)
- Loop increment hardcoded as `x = x + 1`
- Only literal comparisons (can't do `while x < max`)
- No function parameters or return values
- Only `int` type supported in variables

## Architecture Notes

The compiler uses a pattern-matching approach:
- `parse_loop_bound()` - Finds "while" → "<" → parses number
- `parse_var_init()` - Finds "let"/"const" → "=" → parses number
- Each parser function scans source linearly using `ord()` for digit detection

This approach is simple but will need refactoring for full parsing.

## Self-Hosting Estimate

Current: ~10% of features needed
- ✅ Basic control flow
- ✅ Single variable
- ❌ Functions with params/returns
- ❌ Multiple variables
- ❌ Complex expressions
- ❌ String handling

Estimated: 8-10 more major features needed for self-hosting.
