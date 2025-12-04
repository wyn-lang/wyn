# Performance Improvement Action Plan

## Current State

### Benchmark Results (vs Python)
- ✅ Fibonacci: **2.5x faster** (1.03s vs 2.55s)
- ❌ Arrays: **3.3x slower** (0.85s vs 0.26s) 🔴
- ❌ Math: **2.0x slower** (0.85s vs 0.42s) 🟡

### vs Node.js
- ❌ Fibonacci: **5.2x slower** (1.03s vs 0.20s)
- Note: Expected for AOT vs JIT

## Priority 1: Array Performance 🔴

**Gap**: 3.3x slower than Python (CRITICAL)

### Investigation Steps
1. Profile array_ops.wyn execution
2. Examine generated assembly for array access
3. Check bounds checking overhead
4. Compare memory layout with Python

### Potential Issues
- [ ] Bounds checking on every access
- [ ] Poor memory layout
- [ ] Cache misses
- [ ] Inefficient indexing code

### Solutions
- [ ] Eliminate redundant bounds checks
- [ ] Add array specialization for known sizes
- [ ] Use SIMD for bulk operations
- [ ] Optimize array indexing codegen

**Expected Gain**: 2-3x speedup → Match Python

## Priority 2: Math Operations 🟡

**Gap**: 2.0x slower than Python

### Investigation Steps
1. Check if math functions are being called correctly
2. Examine calling convention overhead
3. Compare with Python's C-based math library

### Potential Issues
- [ ] Function call overhead
- [ ] Not using CPU intrinsics
- [ ] Suboptimal float operations

### Solutions
- [ ] Inline sin/cos/sqrt/etc
- [ ] Use CPU SIMD instructions (NEON on ARM)
- [ ] Optimize float register usage
- [ ] Direct syscall for math functions

**Expected Gain**: 1.5-2x speedup → Match Python

## Priority 3: Function Inlining 🟢

**Gap**: 5x slower than Node.js on recursion

### Solutions
- [ ] Implement function inlining pass
- [ ] Inline small functions (<10 lines)
- [ ] Inline hot functions (called in loops)
- [ ] Tail call optimization

**Expected Gain**: 20-30% on recursive code

## Implementation Plan

### Week 1: Array Investigation
```bash
# 1. Profile array benchmark
./build/stage0 --stage1-opt -S -o array.s benchmarks/array_ops.wyn
# Examine assembly

# 2. Add bounds check elimination
# Implement in Stage 1 optimizer

# 3. Test improvement
# Re-run benchmarks
```

### Week 2: Math Optimization
```bash
# 1. Inline math functions
# Add to Stage 1 optimizer

# 2. Use CPU intrinsics
# Add NEON support for ARM

# 3. Test improvement
# Re-run benchmarks
```

### Week 3: Function Inlining
```bash
# 1. Implement inlining pass
# Add to Stage 1 optimizer

# 2. Inline small functions
# Heuristic: <10 lines, called >1 time

# 3. Test improvement
# Re-run benchmarks
```

## Expected Results After Fixes

| Benchmark | Current | Target | Improvement |
|-----------|---------|--------|-------------|
| Fibonacci | 1.03s | 0.80s | 1.3x faster |
| Arrays | 0.85s | 0.30s | 2.8x faster |
| Math | 0.85s | 0.45s | 1.9x faster |

## Comparison After Fixes

### vs Python
- Fibonacci: 3.2x faster (0.80s vs 2.55s) ✅
- Arrays: Similar (0.30s vs 0.26s) ✅
- Math: Similar (0.45s vs 0.42s) ✅

### vs Node.js
- Fibonacci: 2.5x slower (0.80s vs 0.20s)
- Still slower, but acceptable for AOT compiler

## Tools Needed

### 1. Profiler
```bash
# Use Instruments on macOS
instruments -t "Time Profiler" ./array_wyn
```

### 2. Assembly Inspector
```bash
# Generate and examine assembly
./build/stage0 -S -o output.s input.wyn
```

### 3. Benchmark Suite
```bash
# Automated comparison
./benchmarks/run_comparison.sh
```

## Success Criteria

### Must Have
- [ ] Arrays within 20% of Python
- [ ] Math within 20% of Python
- [ ] Maintain 2x+ advantage over Python on CPU

### Nice to Have
- [ ] Function inlining working
- [ ] SIMD array operations
- [ ] Within 3x of Node.js on recursion

## Timeline

- **Week 1**: Array investigation + fixes
- **Week 2**: Math optimization
- **Week 3**: Function inlining
- **Week 4**: Re-benchmark and document

## Next Steps

1. **Run profiler** on array_ops.wyn
2. **Examine assembly** for array access
3. **Implement bounds check elimination**
4. **Re-benchmark** and measure improvement

## Resources

- Benchmarks: `benchmarks/`
- Results: `benchmarks/LANGUAGE_COMPARISON.md`
- Stage 1 docs: `docs/stage1-*.md`
