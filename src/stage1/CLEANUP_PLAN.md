# Stage1 Directory Cleanup Plan

## Current Situation

**30 files** in src/stage1/ directory - way too many!

Most are:
- Experimental versions
- Intermediate iterations
- Unused prototypes
- Development artifacts

## What We Actually Need

### Essential Files (Keep)

**Stage 2 - Self-Hosting:**
- `minimal_self.wyn` (1.3K) - **THE SELF-HOSTING COMPILER** ⭐

**Stage 1 - Feature Complete:**
- `compiler_v4.wyn` (18K) - Most complete Stage1 compiler
- `lexer.wyn` (6.8K) - Tokenizer
- `parser.wyn` (5.8K) - Parser

**Documentation:**
- `README.md` - Overview
- `ARCHITECTURE.md` - Architecture docs

**Total: 6 files**

### Archive (Move to archive/)

**Experimental Versions:**
- `compiler.wyn` - Old version
- `compiler_v2.wyn` - Iteration 2
- `compiler_v3.wyn` - Iteration 3
- `compiler_v5.wyn` - Simplified attempt
- `compiler_v6.wyn` - Generic attempt
- `compiler_ast.wyn` - AST-based attempt
- `compiler_integrated.wyn` - Integration attempt

**Prototypes:**
- `minimal.wyn` - Early prototype
- `working.wyn` - Hardcoded version
- `stage1_concat.wyn` - Concatenation test
- `stage1_full.wyn` - Full version attempt
- `stage2.wyn` - Early Stage2 attempt

**Unused Components:**
- `ast.wyn` - AST definitions (not used)
- `ast_compiler.wyn` - AST compiler (not used)
- `codegen.wyn` - Code generator (not used)
- `expr_eval.wyn` - Expression evaluator (not used)
- `integrated.wyn` - Integration (not used)
- `integration_example.wyn` - Example (not used)
- `main.wyn` - Old main (not used)
- `parser_new.wyn` - New parser (not used)
- `simple_parser.wyn` - Simple parser (not used)
- `token.wyn` - Token definitions (not used)
- `typechecker.wyn` - Type checker (not used)

**Documentation:**
- `README_AST.md` - AST docs (not relevant)

**Total: 23 files to archive**

## Cleanup Actions

### 1. Create Archive Directory
```bash
mkdir -p src/stage1/archive
```

### 2. Move Experimental Versions
```bash
mv src/stage1/compiler.wyn src/stage1/archive/
mv src/stage1/compiler_v2.wyn src/stage1/archive/
mv src/stage1/compiler_v3.wyn src/stage1/archive/
mv src/stage1/compiler_v5.wyn src/stage1/archive/
mv src/stage1/compiler_v6.wyn src/stage1/archive/
mv src/stage1/compiler_ast.wyn src/stage1/archive/
mv src/stage1/compiler_integrated.wyn src/stage1/archive/
```

### 3. Move Prototypes
```bash
mv src/stage1/minimal.wyn src/stage1/archive/
mv src/stage1/working.wyn src/stage1/archive/
mv src/stage1/stage1_*.wyn src/stage1/archive/
mv src/stage1/stage2.wyn src/stage1/archive/
```

### 4. Move Unused Components
```bash
mv src/stage1/ast.wyn src/stage1/archive/
mv src/stage1/ast_compiler.wyn src/stage1/archive/
mv src/stage1/codegen.wyn src/stage1/archive/
mv src/stage1/expr_eval.wyn src/stage1/archive/
mv src/stage1/integrated.wyn src/stage1/archive/
mv src/stage1/integration_example.wyn src/stage1/archive/
mv src/stage1/main.wyn src/stage1/archive/
mv src/stage1/parser_new.wyn src/stage1/archive/
mv src/stage1/simple_parser.wyn src/stage1/archive/
mv src/stage1/token.wyn src/stage1/archive/
mv src/stage1/typechecker.wyn src/stage1/archive/
mv src/stage1/README_AST.md src/stage1/archive/
```

### 5. Update Tests
Update test scripts to only reference:
- `minimal_self.wyn` (Stage2)
- `compiler_v4.wyn` (Stage1)

## After Cleanup

### src/stage1/ (6 files)
```
src/stage1/
├── minimal_self.wyn          # Stage2 - Self-hosting ⭐
├── compiler_v4.wyn           # Stage1 - Feature complete
├── lexer.wyn                 # Tokenizer
├── parser.wyn                # Parser
├── README.md                 # Overview
└── ARCHITECTURE.md           # Architecture
```

### src/stage1/archive/ (23 files)
```
src/stage1/archive/
├── compiler*.wyn             # Old versions
├── minimal.wyn               # Prototypes
├── working.wyn
├── stage*.wyn
├── ast*.wyn                  # Unused components
├── codegen.wyn
├── expr_eval.wyn
├── integrated.wyn
├── integration_example.wyn
├── main.wyn
├── parser_new.wyn
├── simple_parser.wyn
├── token.wyn
├── typechecker.wyn
└── README_AST.md
```

## Benefits

1. **Clarity** - Only 6 essential files
2. **Simplicity** - Easy to understand structure
3. **Maintenance** - Clear what's important
4. **History** - Old versions preserved in archive

## Verification

After cleanup:
- ✅ Test `test_stage2_self_hosting.sh`
- ✅ Test `test_stage1.sh`
- ✅ Verify builds work
- ✅ Update documentation

---

**Status:** Ready to execute
**Impact:** High (much cleaner codebase)
**Risk:** Low (everything archived, not deleted)
