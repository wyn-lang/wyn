# Stage 2 Compiler Progress

**Goal:** Expand Stage 2 from minimal self-hosting to full language support

## Current Status

### Stage 2 Versions

| Version | Status | Features | Self-Hosting |
|---------|--------|----------|--------------|
| minimal_self.wyn | ✅ Complete | Fixed output only | ✅ Yes |
| stage2_v2.wyn | ✅ Complete | Reads source, fixed output | ✅ Yes |
| stage2_v3_minimal.wyn | ✅ Complete | Reads source, fixed output | ✅ Yes |
| stage2_v3.wyn | ❌ Blocked | Array.len() linker bug | ❌ No |

### Known Issues

1. **Array.len() Linker Error**: Stage 0 compiler generates `_STRUCT_len` symbol incorrectly
   - Workaround: Avoid using `.len()` on arrays in Stage 2 code
   - Long-term: Fix Stage 0 code generation

2. **Limited String Operations**: Need to use `string.substring()` instead of builtin `substring()`
   - All string ops require `import string`

## Next Steps

### Phase 1: Fix Foundation (Priority: HIGH)

1. **Fix Array.len() Bug in Stage 0**
   - Location: Stage 0 code generator
   - Impact: Blocks all array operations in Stage 2
   - Required for: Real parsing

2. **Add Lexer to Stage 2**
   - Tokenize source code
   - Handle: identifiers, keywords, strings, numbers, operators
   - Foundation for all parsing

3. **Add Parser to Stage 2**
   - Parse tokens into AST
   - Start with: print statements only
   - Expand to: variables, expressions, functions

### Phase 2: Core Features (Priority: MEDIUM)

4. **Variable Declarations**
   ```wyn
   let x: int = 42
   const name: str = "test"
   ```

5. **Expressions**
   ```wyn
   let sum: int = 10 + 20
   let result: int = x * 2 + 5
   ```

6. **If Statements**
   ```wyn
   if x > 10 {
       print("big")
   }
   ```

7. **While Loops**
   ```wyn
   while x < 100 {
       x = x + 1
   }
   ```

### Phase 3: Functions (Priority: MEDIUM)

8. **Function Definitions**
   ```wyn
   fn add(a: int, b: int) -> int {
       return a + b
   }
   ```

9. **Function Calls**
   ```wyn
   const result: int = add(5, 3)
   ```

### Phase 4: Advanced Features (Priority: LOW)

10. **Arrays**
11. **Structs**
12. **Imports**
13. **Spawn (concurrency)**

## Incremental Approach

Each feature addition must:
1. Maintain self-hosting
2. Pass all existing tests
3. Add new tests for the feature
4. Document the change

## Testing Strategy

### Unit Tests
- Test each parser component independently
- Test code generation for each feature
- Test edge cases

### Integration Tests
- Compile simple programs
- Verify output correctness
- Test self-compilation

### Regression Tests
- Ensure previous features still work
- Maintain infinite self-compilation
- Check binary compatibility

## Milestones

### Milestone 1: Lexer (1 week)
- ✅ Read source file
- ⏳ Tokenize input
- ⏳ Handle all token types
- ⏳ Test with real Wyn code

### Milestone 2: Parser (2 weeks)
- ⏳ Parse print statements
- ⏳ Parse variable declarations
- ⏳ Parse expressions
- ⏳ Build AST

### Milestone 3: Code Gen (2 weeks)
- ⏳ Generate code for variables
- ⏳ Generate code for expressions
- ⏳ Generate code for control flow
- ⏳ Optimize output

### Milestone 4: Self-Compile Stage 1 (1 month)
- ⏳ Handle all Stage 1 features
- ⏳ Compile Stage 1 compiler
- ⏳ Verify correctness
- ⏳ Performance optimization

## Success Criteria

### Short Term (1 month)
- [ ] Lexer complete
- [ ] Parser handles variables
- [ ] Can compile simple programs
- [ ] Still self-hosting

### Medium Term (3 months)
- [ ] Parser handles all basic features
- [ ] Can compile Stage 1
- [ ] 30% language coverage
- [ ] Good test coverage

### Long Term (6 months)
- [ ] 50% language coverage
- [ ] Can compile most Wyn programs
- [ ] Performance competitive with Stage 0
- [ ] Ready to replace Stage 0 for common use

## Resources Needed

### Documentation
- Lexer specification
- Parser grammar
- Code generation patterns
- ARM64 assembly reference

### Tools
- Debugger for generated code
- Test harness
- Performance profiler
- Diff tool for assembly output

### Examples
- Simple programs to compile
- Expected assembly output
- Test cases for each feature

## Risks and Mitigation

### Risk 1: Complexity Explosion
- **Mitigation:** Add features incrementally
- **Mitigation:** Maintain simple design
- **Mitigation:** Refactor regularly

### Risk 2: Breaking Self-Hosting
- **Mitigation:** Test after each change
- **Mitigation:** Keep minimal_self.wyn as fallback
- **Mitigation:** Version each iteration

### Risk 3: Performance Issues
- **Mitigation:** Profile early
- **Mitigation:** Optimize hot paths
- **Mitigation:** Compare with Stage 0

## Current Blockers

1. **Array.len() bug** - Prevents real parsing implementation
2. **Limited stdlib** - Need more string operations
3. **No debugging tools** - Hard to debug generated code

## Immediate Actions

1. Fix Array.len() bug in Stage 0
2. Create lexer test suite
3. Design parser architecture
4. Write code generation templates

---

**Last Updated:** 2025-12-09
**Status:** Planning Phase
**Next Review:** 2025-12-16
