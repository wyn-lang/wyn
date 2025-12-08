# Builtin Elimination Progress

## Goal
Reduce from 30+ builtins to 1 builtin (print only).

## Current Status: 4 Builtins (87% reduction)

### Remaining Builtins

| Builtin | Status | Path to Removal |
|---------|--------|-----------------|
| `print()` | ✅ KEEP | Target - only global function |
| `assert()` | ⏳ Remove | Conflicts with `test` keyword; rename keyword or use different module |
| `args()` | ⏳ Intrinsic | Make compiler intrinsic (generates inline asm), not exposed as builtin |
| `syscall()` | ⏳ Intrinsic | Make compiler intrinsic, not exposed as builtin |

### Eliminated Builtins (26+)

| Builtin | Replacement | Date |
|---------|-------------|------|
| `ord()` | `string.ord()` | 2025-12-09 |
| `substring()` | Slice syntax `[start:end]` | 2025-12-08 |
| `read_file()` | `io.read_file()` | Earlier |
| `write_file()` | `io.write_file()` | Earlier |
| `exit()` | `os.exit()` | Earlier |
| `system()` | `os.system()` | Earlier |
| `getenv()` | `os.getenv()` | Earlier |
| `sleep_ms()` | `time.sleep_ms()` | Earlier |
| `time_now()` | `time.now()` | Earlier |
| `abs()` | `math.abs()` or `.abs()` | Earlier |
| `min()` | `math.min()` | Earlier |
| `max()` | `math.max()` | Earlier |
| ... | ... | ... |

## Remaining Work

### 1. Remove `assert()` as Builtin

**Problem:** `test` is a reserved keyword for test blocks, causing parser conflicts when using `import test`.

**Solutions:**
1. Rename the `test` keyword to something else (e.g., `testcase`)
2. Use a different module name (e.g., `import testing`)
3. Keep `assert()` as a special case

**Effort:** ~2 hours

### 2. Make `args()` a Compiler Intrinsic

**Current:** `args()` is listed as a builtin but generates inline assembly.

**Change:** Remove from builtin list, keep codegen. Users must use `os.args()`.

**Effort:** ~1 hour

### 3. Make `syscall()` a Compiler Intrinsic

**Current:** `syscall()` is listed as a builtin but generates inline assembly.

**Change:** Remove from builtin list, keep codegen. Only accessible via stdlib modules.

**Effort:** ~1 hour

## Path to 1 Builtin

```
Current:  print, assert, args, syscall  (4 builtins)
Step 1:   print, assert, args           (remove syscall from list)
Step 2:   print, assert                 (remove args from list)
Step 3:   print                         (resolve assert/test conflict)
```

**Total estimated effort:** 4-6 hours

## Self-Hosting Dependency

The Stage 1 compiler currently uses:
- `print()` - for output
- `string.ord()` - for parsing (already using module version)
- `io.read_file()`, `io.write_file()`, `io.append_file()` - for file I/O
- `os.system()`, `os.exit()` - for compilation and exit

Stage 1 does NOT use:
- `assert()` - not needed
- `args()` - not needed (hardcoded input file)
- `syscall()` - uses higher-level io/os modules

**Conclusion:** Stage 1 is already compatible with 1-builtin goal.

## Testing

After each change, verify:
1. Stage 1 compiles: `./build/stage0 -o build/stage1 src/stage1/compiler.wyn`
2. Stage 1 runs: `./build/stage1 && ./build/stage1_output`
3. Core tests pass: `make test`

## Success Criteria

- [ ] Only `print()` exposed as builtin
- [ ] `args()` works via `os.args()` only
- [ ] `syscall()` only accessible via stdlib
- [ ] `assert()` moved to module or keyword renamed
- [ ] All tests pass
- [ ] Stage 1 compiles and runs
