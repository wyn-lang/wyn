# Stage 1 Performance Benchmarks

## Key Results

### 🚀 Constant-Heavy Code: **42% Faster**
```
Stage 0: 1.94s
Stage 1: 1.13s
Speedup: 1.72x
```

### Fibonacci (Recursive): **8% Faster**
```
Stage 0: 0.12s
Stage 1: 0.11s
Speedup: 1.08x
```

### Other Workloads: **Same Performance**
- Math operations: 0.02s (both)
- Array operations: 0.01s (both)

## Why the Improvement?

### Constant Folding in Action
**Before (Stage 0):**
```wyn
let a = 10 + 20 + 30 + 40 + 50  // Computed at runtime
```

**After (Stage 1):**
```wyn
let a = 150  // Pre-computed at compile time
```

When called 10 million times:
- Stage 0: 10M additions
- Stage 1: 0 additions (constant)

## Compile Time Overhead

```
Stage 0: 0.15s
Stage 1: 0.16s
Overhead: +7% (0.01s)
```

**Worth it?** Yes! 0.01s compile time for 0.81s runtime savings = **81x ROI**

## Binary Size

```
Stage 0: 51K
Stage 1: 51K
Change: 0%
```

Dead code elimination removes statements, not entire functions, so binary size is unchanged for these benchmarks.

## Recommendations

### ✅ Use Stage 1 for:
- Math-heavy applications
- Code with constant expressions
- Configuration-driven programs
- Scientific computing
- Any production code

### ⚠️ Stage 0 is fine for:
- Quick prototypes
- I/O-bound scripts
- When compile time is critical

## Command

```bash
# Recommended for production
./build/stage0 --stage1-tc --stage1-opt -o output input.wyn
```

## Full Results

See `benchmarks/STAGE1_RESULTS.md` for detailed analysis.
