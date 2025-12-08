# Stage 1 Development Roadmap

## Current Status

✅ **Stage 1 proof of concept working**
- Compiles hello.wyn
- Generates ARM64 assembly
- Creates working binaries

## Critical Blocker

**stage0 bugs prevent further development:**
- substring() returns wrong values
- String concatenation in functions crashes
- Module-level constants not supported

**Impact:** Cannot expand Stage 1 parser/codegen without these features.

## Two Paths Forward

### Path A: Fix stage0 Bugs (10-15 hours)
Fix the 3 critical bugs, then continue Stage 1 development.

### Path B: Minimal Expansion (5-10 hours)
Work within stage0 limitations:
- No substring() - hardcode values
- No string concat - use io.append_file()
- No constants - use literals

Then once Stage 1 is self-hosting, fix everything in Stage 1.

## Recommendation

**Path B** - Work within limitations, get to self-hosting faster.

## Updated Timeline

### Phase 1: Minimal Stage 1 (10-15 hours)
- [x] Proof of concept ✅
- [ ] Add multiple print statements (2 hours)
- [ ] Add function calls (3 hours)
- [ ] Add basic variables (3 hours)
- [ ] Test with simple programs (2 hours)

### Phase 2: Self-Hosting Attempt (5-10 hours)
- [ ] Try to compile Stage 1 with Stage 1
- [ ] Work around stage0 bugs
- [ ] Get basic self-hosting working

### Phase 3: Fix in Stage 1 (10-15 hours)
- [ ] Once self-hosting, fix substring()
- [ ] Fix string concat
- [ ] Add proper parsing

### Phase 4: Syscalls (10-15 hours)
- [ ] Add syscall() primitive
- [ ] Rewrite stdlib
- [ ] Remove C runtime

### Phase 5: One Builtin (5 hours)
- [ ] Move everything to stdlib
- [ ] Only print() remains

**Total: 40-60 hours**

## Current Blockers

1. **substring()** - Returns wrong values
2. **String concat** - Crashes in functions
3. **Constants** - Not supported at module level

**Workarounds:**
- Use hardcoded values
- Use io.append_file()
- Use literals

## Next Immediate Step

Expand Stage 1 to handle multiple print statements (working within stage0 limitations).

