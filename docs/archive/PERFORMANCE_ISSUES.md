# Performance Issues - Complete Analysis

## Benchmark Results

| Test | Wyn Stage 1 | Python | Go | Node.js | Status |
|------|-------------|--------|-----|---------|--------|
| Fibonacci | 1.03s | 2.55s | - | 0.20s | ✅ Beats Python 2.5x |
| Arrays | 0.85s | 0.26s | - | - | ❌ 3.3x slower |
| Math | 0.85s | 0.42s | - | - | ❌ 2.0x slower |
| Concurrency | Hangs | - | 0.01s | - | ❌ Not implemented |

## Critical Issues

### 🔴 #1: spawn Not Implemented
**Symptom**: Hangs indefinitely
**Root Cause**: Codegen missing for STMT_SPAWN
**Impact**: Concurrency completely broken
**Priority**: CRITICAL

**Solution**: Simple auto-sync model
```wyn
spawn { work() }  # Auto-waits at scope exit
```

**Implementation**:
- Track spawned threads (pthread_t array)
- pthread_create() in STMT_SPAWN codegen
- pthread_join() at scope exit
- ~100 lines of code

**Expected Performance**:
- Sequential: 1.09s
- With spawn: 0.05-0.10s (10-20x speedup)
- vs Go: 5-10x slower (acceptable)

### 🔴 #2: Array Performance
**Symptom**: 3.3x slower than Python (0.85s vs 0.26s)
**Root Cause**: Likely bounds checking on every access
**Impact**: HIGH - arrays are fundamental
**Priority**: CRITICAL

**Solution**: Bounds check elimination
- Detect safe array access (loop with known bounds)
- Skip bounds check codegen
- Add --no-bounds-check flag

**Expected**: 2-3x speedup → 0.30s (match Python)

**Status**: 🔄 Agent working on this

### 🟡 #3: Math Performance
**Symptom**: 2.0x slower than Python (0.85s vs 0.42s)
**Root Cause**: Function call overhead + no inlining
**Impact**: MEDIUM - affects scientific code
**Priority**: HIGH

**Solution**: Inline math functions
- Detect math.sin/cos/sqrt calls
- Generate inline code instead of call
- Use CPU intrinsics (NEON on ARM)

**Expected**: 1.5-2x speedup → 0.45s (near Python)

### 🟢 #4: Recursion vs JIT
**Symptom**: 5.2x slower than Node.js (1.03s vs 0.20s)
**Root Cause**: No JIT, no function inlining
**Impact**: LOW - expected for AOT
**Priority**: MEDIUM

**Solution**: Function inlining
- Inline small functions (<10 lines)
- Inline hot functions (in loops)
- Tail call optimization

**Expected**: 20-30% speedup → 0.80s

## Root Cause Analysis

### Why Arrays are Slow
```asm
# Current (with bounds check)
arr[i]:
    load array_ptr
    load array_len
    load i
    cmp i, len        # Bounds check
    b.ge panic        # Branch if out of bounds
    load arr[i]       # Actual access
```

**Overhead**: 3 extra instructions per access
**In loop**: 1M iterations × 10 elements × 3 = 30M extra instructions

### Why Math is Slow
```asm
# Current (function call)
math.sin(x):
    prepare args
    call map_module_function  # Indirection
    call sinf                 # C library
    return
```

**Overhead**: Function call + indirection
**Better**: Inline or direct call

### Why spawn Hangs
```c
case STMT_SPAWN:
    // TODO: Not implemented
    break;  // Does nothing!
```

**Issue**: No codegen at all

## Implementation Priority

### Week 1 (Critical)
1. **Implement spawn codegen** (manual)
2. **Fix array bounds checks** (agent)
3. **Test both fixes**

### Week 2 (Important)
1. **Inline math functions**
2. **Add --no-bounds-check flag**
3. **Re-benchmark**

### Week 3 (Nice to have)
1. **Function inlining pass**
2. **Loop unrolling**
3. **Final benchmarks**

## Expected Final Results

| Test | Current | Target | vs Python | vs Go |
|------|---------|--------|-----------|-------|
| Fibonacci | 1.03s | 0.80s | 3.2x faster ✅ | - |
| Arrays | 0.85s | 0.30s | Similar ✅ | - |
| Math | 0.85s | 0.45s | Similar ✅ | - |
| Concurrency | Hangs | 0.05s | - | 5-10x slower ⚠️ |

## Concurrency Trade-off

**Go's Model**: Complex but powerful
- WaitGroups, Mutexes, Channels
- Full control
- 109x speedup

**Wyn's Model**: Simple but limited
- Auto-sync on scope exit
- No manual primitives
- Target: 20x speedup

**Decision**: Simplicity over raw performance
- Easier to use
- Safer (no deadlocks)
- Good enough for most use cases

## Action Items

### Immediate
- [ ] Implement spawn codegen (100 lines)
- [ ] Wait for agent (array optimization)
- [ ] Test both fixes

### This Week
- [ ] Inline math.sin/cos/sqrt
- [ ] Add --no-bounds-check flag
- [ ] Re-run all benchmarks

### Next Week
- [ ] Function inlining pass
- [ ] Document performance characteristics
- [ ] Update README with results

## Success Metrics

- [ ] spawn working (no hangs)
- [ ] Arrays within 20% of Python
- [ ] Math within 20% of Python
- [ ] Concurrency 10-20x faster than sequential
- [ ] All tests passing
