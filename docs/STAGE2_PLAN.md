# Stage 2 Compiler Plan

**Goal:** True self-hosting - Stage1 compiles itself

## The Bootstrap Problem

**Current Status:**
- Stage0 (C compiler) can compile compiler_v4.wyn ✅
- compiler_v4 can compile simple programs ✅
- compiler_v4 **cannot** compile itself ❌

**Why?**
compiler_v4.wyn uses features that compiler_v4 doesn't implement:
- String slicing: `src[pos:pos+1]`
- String methods: `src.len()`, `vars.len()`
- Complex expressions
- Multiple function calls per line
- String concatenation in expressions

## Solution: Two Paths

### Path A: Expand compiler_v4 Features (Recommended)

Add just enough features to compiler_v4 so it can compile itself:

**Phase 1: String Operations** (Critical)
- [ ] String slicing `str[start:end]`
- [ ] String methods `.len()`
- [ ] String concatenation in expressions

**Phase 2: Expression Improvements**
- [ ] Nested expressions `16 + idx * 8`
- [ ] Multiple operators in one expression
- [ ] Parentheses support

**Phase 3: Function Call Improvements**
- [ ] Multiple function calls per line
- [ ] Function calls in expressions
- [ ] Chained method calls

**Phase 4: Self-Compile Test**
- [ ] Modify compiler_v4 to read its own source
- [ ] Compile and verify output
- [ ] Compare Stage0 vs Stage1 output

**Estimated Time:** 8-12 hours

### Path B: Simplify compiler_v4 (Faster)

Rewrite compiler_v4 to only use features it can compile:

**Changes Needed:**
- Replace `src[pos:pos+1]` with `substring(src, pos, pos+1)`
- Replace `.len()` with `len(str)` builtin
- Simplify expressions to one operation per line
- Break complex statements into multiple lines

**Estimated Time:** 2-3 hours

## Recommendation

**Start with Path B** (simplification):
1. Create `compiler_v5.wyn` that uses only basic features
2. Verify it compiles simple programs
3. Have it compile itself
4. **Then** incrementally add features (Path A)

This gives us:
- Quick win: True self-hosting in 2-3 hours
- Solid foundation for adding features
- Proof that the approach works

## Next Steps

1. **Create compiler_v5.wyn** - simplified version
2. **Test self-compilation** - v5 compiles v5
3. **Add features incrementally** - one at a time
4. **Verify each step** - ensure self-compilation still works

## Success Criteria

✅ Stage1 compiler can compile its own source code
✅ Output binary works correctly
✅ Can compile other programs too
✅ Foundation for adding more features

## Future: Stage 3+

Once Stage2 is self-hosting:
- Add arrays
- Add for loops
- Add structs
- Add remaining operators
- Eventually reach 100% language coverage
