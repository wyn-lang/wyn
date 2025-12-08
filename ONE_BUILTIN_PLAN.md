# Reducing to 1 Builtin - print() Only

## Current State

**8 builtins:**
1. print() - Output
2. assert() - Testing
3. exit() - Process control
4. args() - Command line arguments
5. int_to_str() - String conversion
6. system() - Shell commands
7. write_file() - File writing
8. substring() - String slicing
9. ord() - Character code

## Target State

**1 builtin:**
1. print() - Output only

**Everything else via imports or syscalls.**

## How to Achieve This

### In Stage 1 (Once Self-Hosting)

Stage 1 will have proper implementations, so we can move everything to stdlib:

**assert() → import test**
```wyn
# std/test.wyn
fn assert(condition: bool) {
    if not condition {
        syscall(SYS_EXIT, 1, 0, 0)  # Exit with error
    }
}
```

**exit() → import os**
```wyn
# std/os.wyn
fn exit(code: int) {
    syscall(SYS_EXIT, code, 0, 0)
}
```

**args() → import os**
```wyn
# std/os.wyn
fn args() -> [str] {
    # Use syscall or read from _wyn_argc/_wyn_argv
    return syscall_args()
}
```

**int_to_str() → import string**
```wyn
# std/string.wyn
fn from_int(n: int) -> str {
    # Implement in pure Wyn
    if n == 0 {
        return "0"
    }
    let result = ""
    # ... digit by digit conversion
    return result
}
```

**system() → import os**
```wyn
# std/os.wyn
fn system(cmd: str) -> int {
    syscall(SYS_SYSTEM, cmd, 0, 0)
}
```

**write_file() → import io**
```wyn
# std/io.wyn
fn write_file(path: str, content: str) -> bool {
    const fd = syscall(SYS_OPEN, path, O_WRONLY | O_CREAT, 0644)
    syscall(SYS_WRITE, fd, content, content.len())
    syscall(SYS_CLOSE, fd, 0, 0)
    return true
}
```

**substring() → string method**
```wyn
# Built into string type
const s = "hello"
const sub = s[1:4]  # "ell"
```

**ord() → import string**
```wyn
# std/string.wyn
fn ord(c: str) -> int {
    # Get byte value
    return syscall_ord(c)
}
```

## Implementation Plan

### Phase 1: Add Syscalls to Stage 1 (5 hours)

Add `syscall()` primitive to Stage 1 compiler:

```wyn
# Stage 1 codegen
fn generate_syscall(args: [Expr]) -> str {
    # Generate ARM64 syscall
    let asm = ""
    asm = asm + "    mov x16, x0\n"  # syscall number
    asm = asm + "    mov x0, x1\n"   # arg1
    asm = asm + "    svc #0\n"       # make syscall
    return asm
}
```

### Phase 2: Rewrite Stdlib (10 hours)

Rewrite all stdlib modules to use syscalls instead of C functions.

### Phase 3: Update Stage 1 Compiler (3 hours)

Remove dependencies on builtins:
- Use `import test` for assert
- Use `import os` for exit, args, system
- Use `import string` for int_to_str, substring, ord
- Use `import io` for write_file

### Phase 4: Reduce Builtins in stage0 (2 hours)

Once Stage 1 doesn't need them:
```c
static bool tc1_is_builtin(const char* name) {
    return strcmp(name, "print") == 0;  // Only print
}
```

### Phase 5: Test (2 hours)

Verify everything works with only print() as builtin.

**Total: 22 hours**

## Timeline

**Current:** 8 builtins (stage0 limitations)
**After Stage 1 self-hosting:** 1 builtin (print only)

**Steps:**
1. Complete Stage 1 (35-55 hours)
2. Add syscalls (22 hours)
3. **Result:** 1 builtin only

**Total: 57-77 hours**

## Why We Have 8 Now

**Temporary workarounds for stage0 bugs:**
- stage0 can't handle complex string operations
- stage0 has array indexing bugs
- Stage 1 needs these to work around stage0 limitations

**Once Stage 1 is self-hosting:**
- Stage 1 has no bugs (we control it)
- Can implement everything in pure Wyn
- Reduce to 1 builtin (print only)

## Final Result

```wyn
# Only print() is global

import test
import os
import io
import string

fn main() {
    print("Hello!")  # Only builtin
    
    const data = io.read_file("file.txt")  # Syscall
    test.assert(data.len() > 0)  # Pure Wyn
    os.exit(0)  # Syscall
}
```

**1 builtin. Everything else via imports and syscalls.**

## Conclusion

We're at 8 builtins now due to stage0 limitations.

Once Stage 1 is self-hosting (35-55 hours), we'll reduce to 1 builtin (print only) via syscalls (22 hours).

**Total: 57-77 hours to 1 builtin.**

The architecture is correct. The implementation is in progress.
