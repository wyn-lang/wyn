# Reducing to 1 Builtin - Implementation Plan

## Current Builtins (9)

1. print() - **KEEP**
2. assert() - Move to std/test.wyn
3. exit() - Move to std/os.wyn (syscall)
4. args() - Move to std/os.wyn
5. int_to_str() - Move to std/string.wyn (pure Wyn)
6. system() - Move to std/os.wyn (syscall)
7. write_file() - Move to std/io.wyn (syscall)
8. substring() - Move to string methods
9. ord() - Move to std/string.wyn
10. syscall() - **KEEP temporarily** (needed for stdlib)

## Step 1: Replace in Stage 1 Compiler

Update Stage 1 to use imports instead of builtins:

```wyn
# Before
fn main() {
    const source: str = io.read_file("file.wyn")
    write_file("out.s", asm)
    const result: int = system("cc -o out out.s")
    if result != 0 {
        exit(1)
    }
}

# After
import io
import os

fn main() {
    const source: str = io.read_file("file.wyn")
    io.write_file("out.s", asm)
    const result: int = os.system("cc -o out out.s")
    if result != 0 {
        os.exit(1)
    }
}
```

## Step 2: Update stage0 Builtins

Remove from builtin list:
- exit (use os.exit)
- system (use os.system)
- write_file (use io.write_file)
- int_to_str (use string.from_int)

Keep only:
- print
- syscall (for stdlib)

## Step 3: Update Tests

Update all tests to use imports:
```wyn
# Before
fn main() {
    assert(x > 0)
    exit(0)
}

# After
import test
import os

fn main() {
    test.assert(x > 0)
    os.exit(0)
}
```

## Step 4: Final Reduction

Once everything works with syscalls:
- Remove syscall from builtins (make it a compiler intrinsic)
- Only print() remains as true builtin

## Timeline

- Step 1: Update Stage 1 (2 hours)
- Step 2: Update stage0 (1 hour)
- Step 3: Update tests (3 hours)
- Step 4: Final reduction (1 hour)

**Total: 7 hours**

## Testing Strategy

1. Test each stdlib module works with syscalls
2. Update Stage 1 to use imports
3. Compile Stage 1 with updated code
4. Run test suite
5. Verify all 64 tests pass

## Success Criteria

- [ ] Only print() is builtin
- [ ] All stdlib uses syscalls
- [ ] Stage 1 compiles with imports
- [ ] All tests pass
- [ ] Pure Wyn achieved
