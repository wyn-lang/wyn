# Stage 1 Compiler - Quick Start Guide

## What's New

Stage 1 compiler now parses actual values from source code instead of hardcoding them!

## Try It Out

### 1. Compile Stage 1
```bash
./build/stage0 -o build/stage1 src/stage1/compiler.wyn
```

### 2. Create a Test Program
```bash
cat > examples/stage1_test.wyn << 'EOF'
fn main() {
    let x: int = 10
    while x < 20 {
        print("Counting from 10 to 20")
        x = x + 1
    }
}
EOF
```

### 3. Compile with Stage 1
```bash
./build/stage1
```

### 4. Run the Output
```bash
./build/stage1_output
```

You should see "Counting from 10 to 20" printed 10 times!

## What Works Now

✅ **Dynamic loop bounds** - `while x < 100` parses 100 from source
✅ **Dynamic initialization** - `let x: int = 50` parses 50 from source  
✅ **Const declarations** - `const max: int = 10` works
✅ **Any integer values** - Not limited to small numbers

## Test Suite

Run all tests:
```bash
./test_stage1_improvements.sh
```

## Examples

See `examples/stage1_tests/` for test programs:
- `test1_basic_loop.wyn` - Simple 0 to 3 loop
- `test2_large_bound.wyn` - Loop to 100
- `test3_nonzero_init.wyn` - Start at 50
- `test4_const.wyn` - Using const

## Current Limitations

⚠️ Only one variable supported (named `x` or similar)
⚠️ Only `int` type
⚠️ Loop increment must be `x = x + 1`
⚠️ Comparison must be literal (can't use `while x < max`)
⚠️ No function parameters or returns yet

## Next Steps

See `STAGE1_PROGRESS.md` for roadmap to self-hosting.
