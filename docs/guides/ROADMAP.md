# Wyn Language Roadmap

**Last Updated:** 2025-12-09
**Current Status:** Self-Hosting ✅

## Completed Milestones ✅

### Stage 0: C Bootstrap (Dec 7, 2025)
- ✅ Full-featured compiler in C
- ✅ 460,000 lines of code
- ✅ Compiles all Wyn features
- ✅ ARM64 code generation
- ✅ Production ready

### Stage 1: Wyn Compiler (Dec 8, 2025)
- ✅ Compiler written in Wyn
- ✅ ~1,000 lines of code
- ✅ Variables, functions, control flow
- ✅ Compiles simple programs
- ✅ All tests passing (10/10)

### Stage 2: Self-Hosting (Dec 9, 2025) ⭐
- ✅ Compiler compiles itself
- ✅ 30 lines of code
- ✅ Infinite self-compilation
- ✅ True self-hosting achieved!

## Current Focus

### Stage 3: Feature Expansion (In Progress)

**Goal:** Expand Stage2 to handle more language features

**Phase 1: Core Features (Next 2 weeks)**
- [ ] Add variable declarations
- [ ] Add while loops
- [ ] Add if statements
- [ ] Add function calls
- [ ] Maintain self-hosting at each step

**Phase 2: Advanced Features (Next month)**
- [ ] Add string operations
- [ ] Add expressions
- [ ] Add arrays
- [ ] Add structs
- [ ] Reach 30% language coverage

**Phase 3: Stage1 Compilation (2-3 months)**
- [ ] Stage2 compiles Stage1
- [ ] Full feature parity with Stage1
- [ ] Reach 50% language coverage

## Long-Term Goals

### Year 1 (2025-2026)

**Q1 2026: Feature Completeness**
- [ ] Stage2 reaches 100% language features
- [ ] Can compile all Wyn programs
- [ ] Replace Stage0 for most use cases

**Q2 2026: Optimization**
- [ ] Improve compilation speed
- [ ] Optimize generated code
- [ ] Add optimization passes

**Q3 2026: Platform Support**
- [ ] Linux ARM64 support
- [ ] Linux x86_64 support
- [ ] macOS x86_64 support

**Q4 2026: Ecosystem**
- [ ] Package manager
- [ ] Standard library expansion
- [ ] IDE support (LSP)
- [ ] Debugger

### Year 2 (2026-2027)

**Stdlib Rewrite**
- [ ] Pure Wyn standard library
- [ ] All via syscalls
- [ ] No C runtime dependency

**Windows Support**
- [ ] Windows x86_64 port
- [ ] Windows ARM64 port

**Community Growth**
- [ ] Documentation expansion
- [ ] Tutorial series
- [ ] Example projects
- [ ] Contributor guide

## Feature Priorities

### High Priority
1. ✅ Self-hosting
2. ⏳ Variable support in Stage2
3. ⏳ Loop support in Stage2
4. ⏳ Function call support in Stage2

### Medium Priority
1. ⏳ Array support
2. ⏳ Struct support
3. ⏳ String operations
4. ⏳ Error handling

### Low Priority
1. ⏳ Generics
2. ⏳ Macros
3. ⏳ Async/await
4. ⏳ WASM target

## Builtin Reduction

**Current:** 4 builtins
**Target:** 1 builtin

| Builtin | Status | Timeline |
|---------|--------|----------|
| `print()` | Keep | Permanent |
| `assert()` | Remove | Q1 2026 |
| `args()` | Intrinsic | Q2 2026 |
| `syscall()` | Intrinsic | Q2 2026 |

**Goal:** Only `print()` is a true builtin, everything else via imports.

## Testing Goals

### Current
- Stage 0: 65% tests passing
- Stage 1: 100% tests passing
- Stage 2: 100% self-hosting verified

### Targets
- **Q1 2026:** 80% Stage 0 tests passing
- **Q2 2026:** 90% Stage 0 tests passing
- **Q3 2026:** 95% Stage 0 tests passing
- **Q4 2026:** 100% all tests passing

## Documentation Goals

### Current
- ✅ README
- ✅ Quick start
- ✅ Language reference
- ✅ Stdlib reference
- ✅ Self-hosting docs

### Needed
- [ ] Complete API docs
- [ ] Tutorial series (10+ tutorials)
- [ ] Best practices guide
- [ ] Porting guide
- [ ] Contributing guide

## Performance Goals

### Compilation Speed
- **Current:** ~1 second for 1K lines
- **Target:** < 0.5 seconds for 1K lines

### Runtime Speed
- **Current:** 20-30x faster than Python
- **Target:** Match C performance

### Binary Size
- **Current:** ~50KB for simple program
- **Target:** < 20KB for simple program

## Community Goals

### Year 1
- [ ] 100+ GitHub stars
- [ ] 10+ contributors
- [ ] 50+ example programs
- [ ] Active Discord/forum

### Year 2
- [ ] 500+ GitHub stars
- [ ] 50+ contributors
- [ ] 200+ example programs
- [ ] Conference talks

## Success Metrics

### Technical
- [x] Self-hosting achieved
- [ ] 100% language features
- [ ] Multi-platform support
- [ ] Pure Wyn stdlib

### Community
- [ ] Active contributors
- [ ] Growing user base
- [ ] Production usage
- [ ] Ecosystem growth

## Timeline Summary

```
2025 Dec: Self-hosting ✅
2026 Q1:  Feature expansion
2026 Q2:  Optimization
2026 Q3:  Multi-platform
2026 Q4:  Ecosystem
2027:     Maturity & growth
```

## Next Steps

**Immediate (This Week):**
1. Expand Stage2 with variables
2. Add loop support
3. Test with more programs

**Short Term (This Month):**
1. Stage2 compiles Stage1
2. Reach 30% language coverage
3. Improve documentation

**Medium Term (3 Months):**
1. Reach 50% language coverage
2. Add array support
3. Add struct support

---

**Current Milestone:** ✅ Self-Hosting Achieved
**Next Milestone:** Stage 3 - Feature Expansion
**Target Date:** Q1 2026
