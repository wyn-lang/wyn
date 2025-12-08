# Stage 1 Compiler Improvements

## Summary

The Stage 1 compiler has been enhanced to **parse actual values from source code** instead of using hardcoded constants. This is a critical milestone toward self-hosting.

## What Changed

### 1. Dynamic Loop Bound Parsing ✅
The compiler now parses the actual comparison value from `while x < N` statements.

**Code:** `parse_loop_bound(source: str) -> int`

**Example:**
```wyn
while x < 100 {  // Compiler extracts 100
    print("Hello")
}
```

Generates: `cmp x0, #100` (not hardcoded `#3`)

### 2. Dynamic Variable Initialization ✅
The compiler now parses the initial value from variable declarations.

**Code:** `parse_var_init(source: str) -> int`

**Example:**
```wyn
let x: int = 42  // Compiler extracts 42
```

Generates: `mov x0, #42` (not hardcoded `#0`)

### 3. Const Declaration Support ✅
Parser recognizes both `let` and `const` keywords.

**Example:**
```wyn
const max: int = 100  // Now supported
```

## Test Results

All tests passing (4/4):

```bash
$ ./test_stage1_improvements.sh

✅ Test 1: Basic loop (0 to 3) - 3 iterations
✅ Test 2: Large bound (0 to 100) - 100 iterations
✅ Test 3: Non-zero init (50 to 55) - 5 iterations
✅ Test 4: Const declaration (7 to 9) - 2 iterations
```

## Quick Start

```bash
# 1. Compile Stage 1
./build/stage0 -o build/stage1 src/stage1/compiler.wyn

# 2. Try the showcase example
cp examples/stage1_showcase.wyn examples/stage1_test.wyn
./build/stage1
./build/stage1_output

# 3. Run all tests
./test_stage1_improvements.sh
```

## Files

- `src/stage1/compiler.wyn` - Enhanced compiler with new parsers
- `examples/stage1_tests/` - Test suite (4 tests)
- `test_stage1_improvements.sh` - Automated test runner
- `STAGE1_PROGRESS.md` - Detailed progress tracking
- `STAGE1_QUICKSTART.md` - Quick start guide
- `STAGE1_VISUAL_SUMMARY.txt` - Visual before/after comparison

## Implementation

Both parsers use a simple linear scan:

1. Find keyword (`while`, `let`, or `const`)
2. Skip to delimiter (`<` or `=`)
3. Parse digits using `ord()` to check ASCII codes (48-57)
4. Build number: `result = result * 10 + (code - 48)`

This approach is minimal and works with current Stage 0 capabilities.

## Current Limitations

- Only one variable supported (hardcoded stack offset)
- Only `int` type
- Loop increment must be `x = x + 1` (hardcoded)
- Comparison must be against literal (can't use `while x < max`)
- No function parameters or return values

## Next Steps

Priority features for self-hosting:

1. **Multiple variable support** - Track multiple vars with different stack offsets
2. **Function parameters** - Parse and pass arguments
3. **Return statements** - Parse `return expr` and generate code
4. **Variable references** - Support `while x < max` (not just literals)
5. **Assignment parsing** - Parse `x = expr` dynamically

See `STAGE1_PROGRESS.md` for complete roadmap.

## Impact

**Progress toward self-hosting: ~10% → ~15%**

The compiler can now parse and extract values from source code, which is fundamental for a real compiler. This demonstrates the feasibility of the self-hosting approach.

## Verification

To verify the improvements work:

```bash
# Create a test with custom values
cat > examples/stage1_test.wyn << 'EOF'
fn main() {
    let x: int = 25
    while x < 30 {
        print("Custom values!")
        x = x + 1
    }
}
EOF

# Compile and run
./build/stage1
./build/stage1_output  # Should print 5 times

# Check generated assembly
grep -E "(mov x0, #|cmp x0, #)" build/stage1_output.s
# Should show: mov x0, #25 and cmp x0, #30
```

## Conclusion

The Stage 1 compiler is now significantly more capable. It can parse actual values from source code rather than relying on hardcoded constants. This is a critical step toward the goal of self-hosting, where Stage 1 will compile its own source code.
