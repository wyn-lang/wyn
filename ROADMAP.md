# Wyn Language Roadmap

## Current State
- ✅ Stage 1 compiler complete
- ✅ 47/48 tests passing
- ✅ Performance: 2-3x behind C/Go, matches Java/Node, 20x faster than Python
- ✅ Startup: 2nd fastest (1.29ms)

## High Priority (Can Parallelize)

### 1. Function Inlining (CRITICAL)
**Impact**: 20-30% performance improvement, close gap with Go
**Effort**: ~200 lines
**Complexity**: Medium
**Delegate**: Yes - Agent can implement

**Task**: Add function inlining pass to Stage 1 optimizer
- Inline small functions (<10 lines)
- Inline functions called in loops
- Inline recursive tail calls
- Target: Match Go performance (close 2-5x gap)

### 2. Proper Concurrency (CRITICAL)
**Impact**: Enable true parallelism (10-20x speedup)
**Effort**: ~150 lines
**Complexity**: Medium
**Delegate**: No - Needs careful design

**Task**: Implement pthread-based spawn
- pthread_create in STMT_SPAWN codegen
- Auto-join at scope exit
- Thread-safe shared variables
- Target: 10-20x speedup over sequential

## Medium Priority (Sequential)

### 3. Loop Unrolling
**Impact**: 5-10% improvement on loops
**Effort**: ~100 lines
**Complexity**: Low
**Delegate**: Yes

**Task**: Unroll simple loops with known iteration count
- Detect loops with constant bounds
- Unroll 2x or 4x
- Reduce loop overhead

### 4. Better Register Allocation
**Impact**: 10-20% improvement overall
**Effort**: ~300 lines
**Complexity**: High
**Delegate**: No - Complex

**Task**: Improve register usage in codegen
- Track register liveness
- Reduce stack spills
- Better instruction selection

## Low Priority (Future)

### 5. SIMD Operations
**Impact**: 2-4x on array operations
**Effort**: ~200 lines
**Complexity**: Medium

### 6. LLVM Backend (Stage 3)
**Impact**: Match C performance
**Effort**: ~2000 lines
**Complexity**: Very High

## Parallelization Strategy

### I Can Do (Manual)
1. **Concurrency implementation** - Needs design decisions
2. **Register allocation** - Complex, needs expertise
3. **Architecture decisions** - Can't delegate

### Agent Can Do (Parallel)
1. **Function inlining** - Well-defined task
2. **Loop unrolling** - Straightforward optimization
3. **Benchmark implementations** - Mechanical work

## Immediate Action Plan

### Today
**Me**: Design and implement pthread spawn (~150 lines)
**Agent**: Implement function inlining (~200 lines)

### This Week
**Me**: Test concurrency, integrate inlining
**Agent**: Implement loop unrolling (~100 lines)

### Next Week
**Me**: Better register allocation
**Agent**: Create remaining benchmarks

## Expected Results

### After Function Inlining
- Fibonacci: 0.10s → 0.07s (match Go's 0.04s closer)
- Primes: 0.14s → 0.10s (match Node.js)
- **Gap with Go**: 2-5x → 1.5-3x

### After Concurrency
- Sequential: 1.09s
- With spawn: 0.05-0.10s (10-20x speedup)
- **vs Go**: 5-10x slower (acceptable)

### After Both
- **Match or beat Java/Node.js** ✅
- **Within 1.5-3x of Go** ✅
- **Maintain 20x advantage over Python** ✅

## Success Criteria

- [ ] Function inlining working (20-30% improvement)
- [ ] Concurrency working (10-20x speedup)
- [ ] Within 2x of Go on CPU benchmarks
- [ ] All tests passing
- [ ] Production ready

## Timeline

- **Today**: Start function inlining (agent) + concurrency (me)
- **Week 1**: Complete both, test, benchmark
- **Week 2**: Loop unrolling, polish
- **Week 3**: Release v1.0

## Priority Order

1. 🔴 **Function inlining** - Biggest performance win
2. 🔴 **Concurrency** - Critical missing feature
3. 🟡 **Loop unrolling** - Nice improvement
4. 🟢 **Register allocation** - Advanced optimization
