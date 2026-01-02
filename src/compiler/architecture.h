/*
 * Wyn Compiler Architecture
 * 
 * This document describes the modular structure of the compiler.
 * Currently implemented in compiler.c, ready for extraction.
 */

#ifndef WYN_COMPILER_H
#define WYN_COMPILER_H

/*
 * COMPILER PHASES (in order):
 * 
 * 1. LEXER (Lines 71-624, ~550 lines)
 *    - Tokenization
 *    - String/number parsing
 *    - Keyword recognition
 *    Entry: lexer_new(), lexer_next()
 * 
 * 2. PARSER (Lines 625-2922, ~2300 lines)
 *    - AST construction
 *    - Expression parsing
 *    - Statement parsing
 *    Entry: parse_module()
 * 
 * 3. TYPE CHECKER (Lines 2923-5310, ~2400 lines)
 *    - Type inference
 *    - Type validation
 *    - Symbol resolution
 *    Entry: typecheck_module()
 * 
 * 4. CODEGEN (Lines 5311-10545, ~5200 lines)
 *    - LLVM IR generation
 *    - Optimization
 *    - Output generation
 *    Entry: codegen_module()
 * 
 * 5. MODULE SYSTEM (Lines 5494-5700, ~200 lines)
 *    - Import resolution
 *    - Module loading
 *    - Dependency management
 *    Entry: resolve_imports()
 */

/*
 * EXTRACTION PLAN:
 * 
 * Phase 1: Headers (DONE)
 * - compiler/lexer.h
 * - compiler/parser.h
 * - compiler/checker.h
 * - compiler/codegen.h
 * 
 * Phase 2: Implementation (TODO)
 * - Extract each phase to separate .c file
 * - Update includes
 * - Test after each extraction
 * 
 * Phase 3: Integration (TODO)
 * - Integrate arena allocator
 * - Add error recovery
 * - Improve modularity
 */

/*
 * DEPENDENCIES:
 * 
 * Lexer: None (standalone)
 * Parser: Lexer
 * Checker: Parser, Lexer
 * Codegen: Checker, Parser, Lexer
 * Module: All phases
 */

#endif // WYN_COMPILER_H
