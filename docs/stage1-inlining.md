# Stage 1 Function Inlining

Function inlining optimization for the Wyn Stage 1 compiler.

## Overview

The inlining pass replaces small function calls with their body, eliminating call overhead and enabling further optimizations.

## Strategy

**Inline when:**
- Function body has < 10 statements
- Function is non-recursive
- Function has no spawn/defer statements
- Function is called at least once

**Don't inline:**
- Recursive functions
- Functions with complex control flow
- Large functions (≥10 statements)
- Functions with spawn/defer

## Implementation

Located in `bootstrap/stage0.c` after the bounds check optimizer (~line 3840):

1. **`should_inline()`** - Checks if function meets inlining criteria
2. **`inline_function_call()`** - Replaces EXPR_CALL with function body
3. **`inline_pass()`** - Walks AST and inlines eligible calls
4. **Called from** - `optimize_module()` in Stage 1 optimization pipeline

## Performance

**Benchmark results:**

```bash
# Simple helper functions (add, mul, compute)
./build/stage0 -o bench_no_opt inline_bench.wyn
./build/stage0 --stage1-opt -o bench_opt inline_bench.wyn

# Without optimization: 0.164s
# With optimization:    0.118s
# Improvement:          28% faster
```

## Usage

```bash
# Enable with Stage 1 optimizations
./build/stage0 --stage1-opt -o output input.wyn

# Or with type checker + optimizations
./build/stage0 --stage1-tc --stage1-opt -o output input.wyn
```

## Example

**Before inlining:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    const x: int = add(5, 10)  // Function call
}
```

**After inlining:**
```wyn
fn main() {
    const x: int = 5 + 10  // Inlined expression
}
```

## Limitations

- Only inlines simple return expressions (single STMT_RETURN)
- Doesn't inline functions with multiple statements yet
- No cross-module inlining
- No inline depth limit (could cause code bloat)

## Future Improvements

- Inline multi-statement functions
- Cost model for inlining decisions
- Inline depth limit
- Cross-module inlining
- Profile-guided inlining
