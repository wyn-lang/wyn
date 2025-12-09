# Session 6: Stage2 Planning

**Date:** 2025-12-09
**Duration:** 30 minutes
**Focus:** Stage2 self-hosting analysis and planning

## Achievements

### 1. Runtime Analysis ✅
**Question:** Do we still need the `runtime/` directory?

**Answer:** Yes, for now
- Runtime provides C implementations for spawn, GUI, GPU, arrays
- Stage0 links these when compiling programs that use those features
- Stage1 compiler doesn't use these features (pure computation)
- Keep for testing full language capabilities
- Eventually replace with pure Wyn + syscalls

**Files in runtime/:**
- `builtins.c` - File I/O functions
- `spawn.c` - Concurrency/threading
- `array.c` - Array operations
- `gui_*.c` - GUI support (macOS, Windows, Linux)
- `gpu_*.c` - GPU support (Metal, Vulkan, CUDA)
- `mobile_*.m/.c` - Mobile platforms

### 2. Language Coverage Analysis ✅
**Question:** Does Stage1 cover all language features?

**Answer:** No - only ~15-20% of full spec

**Stage1 Supports:**
- Variables (let, const)
- Basic expressions (arithmetic, comparisons)
- Control flow (if, while)
- Functions (definitions, calls)
- Print statements
- String literals

**Missing from Full Spec:**
- Structs and methods
- Arrays (declaration, indexing, iteration)
- For loops
- Floats, booleans
- Spawn/concurrency
- Imports (parsed but not processed)
- Logical/bitwise operators
- String interpolation
- Method calls
- Slicing syntax
- Match expressions
- Generics, enums, lambdas

**Conclusion:** Stage1 is a minimal viable compiler, not feature-complete

### 3. Stage2 Bootstrap Analysis ✅
**Discovery:** compiler_v4 cannot compile itself

**The Bootstrap Problem:**
```wyn
# compiler_v4.wyn uses features it doesn't implement:
while pos < src.len() {              # ❌ .len() method
    if src[pos:pos+1] == "let" {     # ❌ String slicing
        result = temp + "," + name    # ❌ Complex concatenation
    }
}
```

**Feature Gap:**
| Feature | Used by v4 | Implemented by v4 |
|---------|------------|-------------------|
| String slicing `[:]` | ✅ | ❌ |
| String methods `.len()` | ✅ | ❌ |
| Complex expressions | ✅ | ❌ |
| Nested function calls | ✅ | ❌ |

### 4. Stage2 Strategy Defined ✅

**Two Paths Identified:**

**Path A: Add Features** (8-12 hours)
- Implement string slicing
- Implement method calls
- Support complex expressions
- More capable but riskier

**Path B: Simplify Compiler** (2-3 hours) ⭐ **RECOMMENDED**
- Create compiler_v5 using only basic features
- Replace `str[i:j]` with `substring(str, i, j)`
- Replace `.len()` with `len(str)` builtin
- Break complex expressions into simple steps
- Quick win, proves self-hosting works

**Decision:** Start with Path B, then add features incrementally

## Deliverables

### Documentation Created
1. **`docs/STAGE2_PLAN.md`** - Detailed implementation plan
2. **`docs/STAGE2_STATUS.md`** - Current status and strategy
3. **`test_stage2_bootstrap.sh`** - Bootstrap test script
4. **Updated README.md** - Stage2 section added

### Key Insights Documented
- Runtime is still needed for full language features
- Stage1 covers ~15-20% of language spec
- compiler_v4 uses features it can't compile
- Simplification path is faster to self-hosting

## Next Steps

### Immediate (Next Session)
1. **Create compiler_v5.wyn**
   - Simplify all complex features
   - Use only: variables, if, while, functions, print
   - Replace slicing with substring()
   - Replace .len() with len() builtin

2. **Test Self-Compilation**
   - Compile v5 with Stage0
   - Have v5 compile itself
   - Verify output works

3. **Validation**
   - Compare outputs
   - Test with examples
   - Document results

### Future Sessions
4. **Add Features Incrementally**
   - String slicing
   - Method calls
   - Complex expressions
   - Maintain self-hosting at each step

5. **Expand Language Coverage**
   - Arrays
   - For loops
   - Structs
   - Eventually 100% coverage

## Timeline

| Milestone | Estimated Time | Status |
|-----------|---------------|--------|
| Stage1 Complete | - | ✅ Done |
| Stage2 Planning | 30 min | ✅ Done |
| compiler_v5 (simplified) | 2-3 hours | ⏳ Next |
| Self-hosting achieved | +1 hour | ⏳ Pending |
| Feature additions | 8-12 hours | ⏳ Future |
| Full language coverage | 40-60 hours | ⏳ Long-term |

## Success Metrics

✅ **Completed This Session:**
- Analyzed runtime requirements
- Assessed language coverage
- Identified bootstrap problem
- Defined clear path forward
- Created comprehensive documentation

🎯 **Next Session Goals:**
- Create compiler_v5.wyn
- Achieve true self-hosting
- Validate with tests

## Technical Notes

### Bootstrap Complexity
The classic compiler bootstrap problem:
1. Compiler uses advanced features
2. Compiler can't compile those features
3. Solution: Either add features OR simplify compiler

### Why Path B First?
- Proves the concept works
- Faster validation
- Easier to debug
- Solid foundation for incremental improvements
- Lower risk

### Long-Term Vision
```
Stage0 (C) → Stage1 (basic) → Stage2 (self-hosting) → Stage3+ (full features)
     ✅              ✅                 🚧                      ⏳
                                   (Next session)
```

## Files Modified

- `README.md` - Added Stage2 status
- `docs/STAGE2_PLAN.md` - Created
- `docs/STAGE2_STATUS.md` - Created
- `test_stage2_bootstrap.sh` - Created
- `docs/session6_stage2_planning.md` - This file

## Conclusion

Stage2 planning is complete. We have a clear, practical path to self-hosting:
1. Simplify compiler to use only basic features (compiler_v5)
2. Achieve self-hosting quickly (2-3 hours)
3. Add features incrementally while maintaining self-hosting

The bootstrap problem is well-understood, and we have a concrete solution. Ready to implement in the next session.
