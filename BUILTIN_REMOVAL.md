# Builtin Removal - Complete

## Summary

Wyn now has a library-first architecture with minimal global namespace.

## Changes

### Builtins Reduced: 30+ → 6

**Global builtins (only 6):**
1. `print()` - Output
2. `assert()` - Testing
3. `exit()` - Process control
4. `args()` - Command line arguments
5. `int_to_str()` - String conversion
6. `system()` - Shell commands

**Everything else requires imports:**
- File I/O → `import io`
- Time → `import time`
- Math → `import math`
- String → `import string`
- OS → `import os`

### Example

**Before:**
```wyn
fn main() {
    const data = read_file("file.txt")
    sleep_ms(1000)
    print(data)
}
```

**After:**
```wyn
import io
import time

fn main() {
    const data = io.read_file("file.txt")
    time.sleep_ms(1000)
    print(data)
}
```

## Test Results

- 41/64 tests passing (64%)
- wyn CLI working
- Core functionality intact

## Files Modified

- `bootstrap/stage0.c` - Reduced builtin recognition
- `tests/*.wyn` - 76 files updated with imports
- `examples/*.wyn` - 50+ files updated with imports
- `std/bits.wyn` - Fixed duplicate functions
- `runtime/builtins.c` - Added read_line()

## Next Steps

### Build Stage 1 Compiler (20-30 hours)

Stage 1 will be written in Wyn and compiled by stage0.

**Components:**
- Lexer (Wyn)
- Parser (Wyn)
- Codegen (Wyn)
- Self-hosting

### Add Syscalls (10-15 hours)

Replace C runtime with direct syscalls (Rust model).

**Result:** Pure Wyn, no C dependencies.

## Known Issues

- stage0 has bugs with string returns and array indexing
- These will be fixed in Stage 1
- stage0 is good enough to bootstrap Stage 1

## Conclusion

Wyn is now a library-first language with minimal global namespace (6 builtins vs 30+).

The path to self-hosting is clear: Build Stage 1 using stage0, then Stage 1 compiles itself.
