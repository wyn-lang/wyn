# Stage 2: Path to True Self-Hosting

**Status:** Planning Phase
**Goal:** Stage1 compiler compiles itself
**Current Blocker:** Feature gap

## Current Situation

### What Works ✅
- **Stage0** (C bootstrap) compiles compiler_v4.wyn
- **compiler_v4** compiles simple programs (variables, loops, if, functions)
- All Stage1 tests passing (10/10)

### The Problem ❌
**compiler_v4 cannot compile itself** because:

| Feature | Used by compiler_v4 | Implemented by compiler_v4 |
|---------|---------------------|----------------------------|
| String slicing `[start:end]` | ✅ Yes | ❌ No |
| String methods `.len()` | ✅ Yes | ❌ No |
| Complex expressions | ✅ Yes | ❌ No |
| Nested function calls | ✅ Yes | ❌ No |
| String concatenation in expressions | ✅ Yes | ❌ No |

**Example from compiler_v4.wyn:**
```wyn
# Uses features it can't compile:
while pos < src.len() {              # .len() method
    if src[pos:pos+1] == "let" {     # String slicing
        result = temp + "," + name    # Complex concatenation
    }
}
```

## Two Paths Forward

### Path A: Add Features to compiler_v4
**Expand compiler to support features it uses**

**Required Features:**
1. String slicing: `str[start:end]` → `substring()` calls
2. Method calls: `.len()` → function calls
3. Complex expressions: `a + b + c` → multiple operations
4. Nested calls: `f(g(x))` → temporary variables

**Pros:**
- Results in more capable compiler
- Closer to full language support
- Better foundation for Stage3+

**Cons:**
- More complex (8-12 hours)
- Higher risk of bugs
- Harder to debug

### Path B: Simplify compiler_v4 → compiler_v5
**Rewrite compiler using only features it implements**

**Changes:**
```wyn
# Before (compiler_v4):
while pos < src.len() {
    if src[pos:pos+1] == "let" {
        result = temp + "," + name
    }
}

# After (compiler_v5):
const src_len: int = len(src)  # Use builtin
while pos < src_len {
    const c: str = substring(src, pos, pos + 1)  # Use builtin
    if c == "l" {
        const temp2: str = temp + ","  # One op per line
        result = temp2 + name
    }
}
```

**Pros:**
- Quick win (2-3 hours)
- Proves self-hosting works
- Easier to debug
- Solid foundation

**Cons:**
- More verbose code
- Still need to add features later
- Temporary step

## Recommendation: Path B First

**Strategy:**
1. **Week 1:** Create compiler_v5 (simplified, self-hosting)
2. **Week 2:** Verify it works, test thoroughly
3. **Week 3+:** Incrementally add features (Path A)

**Benefits:**
- Quick validation of approach
- Working self-hosting compiler as foundation
- Can add features one at a time
- Each step maintains self-hosting

## Implementation Plan (Path B)

### Phase 1: Create compiler_v5.wyn
- [ ] Replace all `str[start:end]` with `substring(str, start, end)`
- [ ] Replace all `.len()` with `len(str)` builtin
- [ ] Break complex expressions into simple steps
- [ ] One operation per line
- [ ] Test with simple programs

### Phase 2: Self-Compilation Test
- [ ] Modify compiler_v5 to read its own source
- [ ] Compile: `stage0 -o compiler_v5 compiler_v5.wyn`
- [ ] Run: `compiler_v5` (should compile itself)
- [ ] Verify output works

### Phase 3: Validation
- [ ] Compare Stage0 output vs Stage1 output
- [ ] Test with all stage1_*.wyn examples
- [ ] Document any differences
- [ ] Fix bugs

### Phase 4: Feature Addition (Path A)
- [ ] Add string slicing
- [ ] Add method calls
- [ ] Add complex expressions
- [ ] Maintain self-hosting at each step

## Success Criteria

✅ **Stage2 Complete When:**
1. compiler_v5 compiles its own source code
2. Output binary works correctly
3. Can compile other programs
4. All tests pass

## Timeline

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Phase 1 | 2-3 hours | compiler_v5.wyn |
| Phase 2 | 1 hour | Self-compilation works |
| Phase 3 | 1 hour | Validation complete |
| **Total** | **4-5 hours** | **True self-hosting** |

## Next Session Goals

1. Create `src/stage1/compiler_v5.wyn`
2. Simplify all complex features
3. Test self-compilation
4. Document results

## Long-Term Vision

```
Stage0 (C) → Stage1 (basic) → Stage2 (self-hosting) → Stage3+ (full features)
     ✅              ✅                 🚧                      ⏳
```

**Stage2 is the critical milestone** - once we have true self-hosting, we can:
- Add features incrementally
- Always maintain self-hosting
- Eventually reach 100% language coverage
- Replace Stage0 entirely
