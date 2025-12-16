# Compiler Cleanup Plan

## Current State

**File:** src/compiler.c
**Size:** 14,118 lines
**Issue:** 78% is unused legacy code (x86/ARM backends)

## Code Distribution

- Lines 1-4,993: Shared code (lexer, parser, type checker) - **KEEP**
- Lines 4,994-11,059: Legacy x86/ARM codegen - **REMOVE** (6,065 lines)
- Lines 11,060-14,118: LLVM codegen - **KEEP** (3,058 lines)

## Cleanup Plan

### Phase 1: Extract Clean Compiler
1. Create src/compiler_llvm.c with only:
   - Lexer/Parser (~2000 lines)
   - Type checker (~1500 lines)  
   - LLVM codegen (~3000 lines)
   - **Total: ~6500 lines (54% reduction)**

2. Move old compiler to src/compiler_legacy.c

### Phase 2: Fix Dependencies
- Ensure all forward declarations are correct
- Fix function ordering issues
- Add missing includes

### Phase 3: Test and Switch
- Build with new compiler
- Run full test suite
- Update Makefile
- Remove legacy file

## Benefits

- **Smaller codebase** - 6.5K vs 14K lines
- **Faster compilation** - Less code to compile
- **Easier maintenance** - Only one backend
- **Clearer code** - No legacy cruft
- **Faster development** - Less to understand

## Timeline

- **Phase 1:** 1 hour (extraction)
- **Phase 2:** 2 hours (fix dependencies)
- **Phase 3:** 1 hour (testing)
- **Total:** 4 hours

## Current Workaround

Using full compiler.c for now. Cleanup can be done later without breaking anything.

## Note

This is a refactoring task - no new features, just code cleanup. Can be done anytime without affecting functionality.
