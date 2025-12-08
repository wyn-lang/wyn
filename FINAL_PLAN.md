# Final Plan - 1 Builtin Only

## Goal

**Only print() as a builtin. Everything else via imports.**

## Current State

- **8 builtins** (temporary, due to stage0 limitations)
- **64 tests** (41 passing, 23 failing)
- **Stage 1** working (proof of concept)

## The Plan

### Phase 1: Complete Stage 1 (35-55 hours)

Build full Stage 1 compiler in Wyn:
- Full parser
- Full codegen
- Self-hosting

**At this point:** Still 8 builtins (Stage 1 uses them)

### Phase 2: Add Syscalls (10-15 hours)

Add `syscall()` primitive to Stage 1:

```wyn
# Stage 1 compiler recognizes syscall() and generates:
fn syscall(num: int, arg1: int, arg2: int, arg3: int) -> int {
    # Generates ARM64:
    # mov x16, x0
    # mov x0, x1
    # mov x1, x2
    # mov x2, x3
    # svc #0
}
```

### Phase 3: Rewrite Stdlib (10-15 hours)

**std/os.wyn:**
```wyn
const SYS_EXIT: int = 1

fn exit(code: int) {
    syscall(SYS_EXIT, code, 0, 0)
}

fn args() -> [str] {
    # Read from global _wyn_argc/_wyn_argv
    # Or use syscall
}

fn system(cmd: str) -> int {
    syscall(SYS_SYSTEM, cmd, 0, 0)
}
```

**std/io.wyn:**
```wyn
const SYS_OPEN: int = 5
const SYS_WRITE: int = 4
const SYS_CLOSE: int = 6

fn write_file(path: str, content: str) -> bool {
    const fd = syscall(SYS_OPEN, path, 0x601, 0644)
    syscall(SYS_WRITE, fd, content, content.len())
    syscall(SYS_CLOSE, fd, 0, 0)
    return true
}
```

**std/string.wyn:**
```wyn
fn from_int(n: int) -> str {
    # Pure Wyn implementation
    if n == 0 {
        return "0"
    }
    let digits = ""
    let num = n
    while num > 0 {
        const digit = num % 10
        digits = chr(48 + digit) + digits
        num = num / 10
    }
    return digits
}

fn chr(code: int) -> str {
    # Use syscall or inline assembly
}

fn ord(c: str) -> int {
    # Use syscall or inline assembly
}

fn substring(s: str, start: int, end: int) -> str {
    # Pure Wyn implementation
    let result = ""
    let i = start
    while i < end {
        result = result + s[i:i+1]
        i = i + 1
    }
    return result
}
```

**std/test.wyn:**
```wyn
fn assert(condition: bool) {
    if not condition {
        print("Assertion failed!")
        syscall(SYS_EXIT, 1, 0, 0)
    }
}
```

### Phase 4: Update Stage 1 Compiler (5 hours)

Remove builtin dependencies:

```wyn
# Before
fn main() {
    const argv = args()
    assert(argv.len() > 0)
    write_file("out.s", asm)
    system("cc -o out out.s")
}

# After
import os
import io
import test

fn main() {
    const argv = os.args()
    test.assert(argv.len() > 0)
    io.write_file("out.s", asm)
    os.system("cc -o out out.s")
}
```

### Phase 5: Reduce Builtins (1 hour)

Update Stage 1 compiler to only recognize print():

```wyn
# In Stage 1 compiler
fn is_builtin(name: str) -> bool {
    return name == "print"  # Only print
}
```

### Phase 6: Test (3 hours)

Verify everything works:
- All tests pass
- Stage 1 compiles itself
- Only print() is builtin

**Total: 64-89 hours from now**

## Tests - Keep or Delete?

**Recommendation: KEEP**

**Why:**
1. **Already working** - 41/64 passing (64%)
2. **Good coverage** - Core language, stdlib, concurrency
3. **Valuable** - Caught bugs, validate features
4. **Fixable** - 23 failing tests can be fixed as Stage 1 develops

**What to do:**
- Keep all 64 tests
- Fix failing tests as Stage 1 adds features
- Add new tests for new features
- Aim for 100% passing

**Don't delete and recreate** - that's wasted work.

## Will This Work?

**YES.** Here's the proof:

**Rust does exactly this:**
- Only `println!()` macro is "special"
- Everything else is stdlib
- Stdlib uses syscalls
- No libc dependency
- Native performance

**Zig does this:**
- Minimal builtins
- Pure Zig stdlib
- Syscalls for I/O
- Works perfectly

**Wyn will do the same:**
- Only `print()` builtin
- Pure Wyn stdlib
- Syscalls for I/O
- Native performance

**No issues down the line.** This is the proven approach.

## Summary

**Current:** 8 builtins (temporary)
**Target:** 1 builtin (print only)
**Path:** Stage 1 → Syscalls → Pure Wyn
**Time:** 64-89 hours
**Tests:** Keep and fix (don't delete)
**Will it work:** Yes (proven by Rust/Zig)

Ready to continue building Stage 1?
