# Internal Development Checklist

## Completed ✅

### Architecture (10 hours)
- [x] Builtin removal (30+ → 8)
- [x] Library-first design
- [x] Module system
- [x] 126 files updated

### Stage 1 Basics (5 hours)
- [x] Proof of concept
- [x] Compiles hello.wyn
- [x] Variables
- [x] Expressions
- [x] Function calls
- [x] If statements

### Bug Fixes (4 hours)
- [x] substring() type inference ✅
- [x] substring() returns correct values ✅
- [x] String concat in functions ✅ FIXED!
- [ ] Array of strings indexing (workaround exists)

## Status Update

**String concat is FIXED!** The type inference fix also fixed string concatenation.

**Remaining bug:** Array indexing for strings (returns garbage)
**Workaround:** Process inline, don't store in arrays

**This means Stage 1 can now:**
- Use string concatenation
- Return strings from functions
- Build complex strings

**Blocker removed!** Stage 1 development can proceed faster.

## Priority Tasks

**P0 - Critical:**
1. Fix string concat (blocks codegen)
2. Fix array handling (blocks parser)

**P1 - High:**
3. Stage 1 expansion (delegated)
4. Self-hosting attempt

**P2 - Medium:**
5. Add syscalls
6. Rewrite stdlib

**P3 - Low:**
7. Reduce to 1 builtin
8. Optimize

## Time Estimates

- String concat fix: 2-3 hours
- Array fix: 2-3 hours
- Stage 1 expansion: 15-20 hours (delegated)
- Self-hosting: 5-10 hours
- Syscalls: 10-15 hours
- One builtin: 5-10 hours

**Total: 39-61 hours**

## Current Focus

- Agent: Expanding Stage 1 features
- Me: Fixing stage0 bugs

**Goal:** Get to self-hosting ASAP
