# Stage 2 Compiler

This directory contains the self-hosting Wyn compiler.

## The Compiler

**File:** `minimal_self.wyn` (1.3K)

The self-hosting compiler that compiles itself.

```bash
./build/stage0 -o build/stage2 src/stage1/minimal_self.wyn
./build/stage2
./build/stage2_output  # Compiles itself again!
```

**Features:**
- 30 lines of code
- Compiles itself infinitely
- True self-hosting ✅

## Supporting Files

- `lexer.wyn` (6.7K) - Tokenizer (for future use)
- `parser.wyn` (5.7K) - Parser (for future use)
- `README.md` - This file
- `ARCHITECTURE.md` - Architecture docs

## Archive

The `archive/` directory contains 26 historical files including all old compiler versions (v1-v6). These are preserved for reference only.

## Usage

```bash
# Compile Stage2
./build/stage0 -o build/stage2 src/stage1/minimal_self.wyn

# Run Stage2 (compiles itself)
./build/stage2

# Run output (compiles itself again)
./build/stage2_output

# Test self-hosting
./test_stage2_self_hosting.sh
```

## Next Steps

Expand `minimal_self.wyn` with more features while maintaining self-hosting.

---

**Status:** ✅ Self-Hosting
**Active Files:** 4
**Archived Files:** 26
