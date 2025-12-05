# Wyn Compiler

## Stage 0 (Bootstrap)

C-based bootstrap compiler. Compiles Wyn to native code.

```bash
make stage0
./build/stage0 -o output input.wyn
```

## Stage 1 (Self-hosted)

Enhanced compiler written in Wyn with:

- **Type checking**: Better error messages, type inference
- **Optimizations**: Constant folding, dead code elimination, inlining

```bash
# Type checking only
wyn --stage1-tc -o output input.wyn

# Optimizations only
wyn --stage1-opt -o output input.wyn

# Both (recommended)
wyn --stage1-tc --stage1-opt -o output input.wyn
```

## Optimizations

- Constant folding
- Dead code elimination
- Function inlining
- Loop optimizations

Performance improvement: 28-42% faster than Stage 0.
