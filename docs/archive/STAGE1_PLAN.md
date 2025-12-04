# Stage 1 Compiler - Implementation Plan

## Goal
Write the Wyn compiler in Wyn itself (self-hosting).

## Status
- Stage 0: ✅ Complete (47/48 tests passing)
- Stage 1: 🚧 Ready to start

## Approach

### Phase 1: Lexer (Week 1)
Complete `src/stage1/lexer.wyn`:
- Tokenize source code
- Handle all 43 keywords
- String literals, numbers, identifiers
- Comments

### Phase 2: Parser (Week 2)
Implement in `src/stage1/parser.wyn`:
- Parse expressions
- Parse statements
- Build AST
- Error reporting

### Phase 3: Type Checker (Week 3)
Implement in `src/stage1/typechecker.wyn`:
- Type inference
- Type checking
- Symbol table
- Error messages

### Phase 4: Code Generator (Week 4)
Implement in `src/stage1/codegen.wyn`:
- Generate ARM64 assembly
- Generate x86_64 assembly
- Optimization passes
- Link with runtime

### Phase 5: Testing & Polish (Week 5)
- Self-hosting test (Stage 1 compiles itself)
- Performance optimization
- Bug fixes
- Documentation

## Success Criteria

1. Stage 1 compiles all 47 tests
2. Stage 1 compiles itself
3. Stage 1 output matches Stage 0 output
4. Performance: 2-5x faster than Stage 0

## Start Date
Ready to begin immediately.

## Resources Needed
- Stage 0 compiler (reference implementation)
- All language features working
- Comprehensive test suite
- Benchmarks for performance tracking

All prerequisites met. Let's build Stage 1!
