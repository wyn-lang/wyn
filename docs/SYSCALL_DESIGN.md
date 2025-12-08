# Syscall Design for Wyn

## Goal

Pure Wyn stdlib with no C dependencies. All I/O via syscalls.

## The syscall() Primitive

```wyn
fn syscall(num: int, arg1: int, arg2: int, arg3: int, arg4: int, arg5: int, arg6: int) -> int
```

**Compiler generates:**
```asm
_syscall:
    mov x16, x0      # syscall number
    mov x0, x1       # arg1
    mov x1, x2       # arg2
    mov x2, x3       # arg3
    mov x3, x4       # arg4
    mov x4, x5       # arg5
    mov x5, x6       # arg6
    svc #0           # make syscall
    ret
```

## macOS ARM64 Syscalls

```wyn
# Syscall numbers for macOS ARM64
const SYS_EXIT: int = 1
const SYS_READ: int = 3
const SYS_WRITE: int = 4
const SYS_OPEN: int = 5
const SYS_CLOSE: int = 6
```

## Pure Wyn Stdlib

### std/os.wyn
```wyn
fn exit(code: int) {
    syscall(1, code, 0, 0, 0, 0, 0)
}

fn args() -> [str] {
    # Read from _wyn_argc/_wyn_argv globals
    # Or implement via syscall
}
```

### std/io.wyn
```wyn
fn write_file(path: str, content: str) -> bool {
    const fd: int = syscall(5, path, 0x601, 0644, 0, 0, 0)  # open
    if fd < 0 {
        return false
    }
    syscall(4, fd, content, content.len(), 0, 0, 0)  # write
    syscall(6, fd, 0, 0, 0, 0, 0)  # close
    return true
}

fn read_file(path: str) -> str {
    const fd: int = syscall(5, path, 0, 0, 0, 0, 0)  # open
    if fd < 0 {
        return ""
    }
    # Allocate buffer
    const buf: str = alloc(4096)
    const n: int = syscall(3, fd, buf, 4096, 0, 0, 0)  # read
    syscall(6, fd, 0, 0, 0, 0, 0)  # close
    return buf
}
```

### std/string.wyn
```wyn
fn from_int(n: int) -> str {
    if n == 0 {
        return "0"
    }
    
    let result: str = ""
    let num: int = n
    let negative: int = 0
    
    if num < 0 {
        negative = 1
        num = 0 - num
    }
    
    while num > 0 {
        const digit: int = num % 10
        const c: str = chr(48 + digit)
        result = c + result
        num = num / 10
    }
    
    if negative == 1 {
        result = "-" + result
    }
    
    return result
}

fn chr(code: int) -> str {
    # Allocate 2-byte string
    const buf: str = alloc(2)
    # Set first byte to code
    # Set second byte to 0
    return buf
}

fn ord(c: str) -> int {
    # Get first byte of string
    # Return as int
}
```

## Implementation Plan

### Phase 1: Add syscall() to Stage 1 (3 hours)
- Recognize syscall() in codegen
- Generate ARM64 syscall instruction
- Test with simple syscall

### Phase 2: Rewrite std/os.wyn (2 hours)
- exit() via syscall
- args() via globals or syscall
- system() via syscall

### Phase 3: Rewrite std/io.wyn (3 hours)
- read_file() via syscalls
- write_file() via syscalls
- append_file() via syscalls

### Phase 4: Rewrite std/string.wyn (2 hours)
- from_int() in pure Wyn
- chr() via memory manipulation
- ord() via memory read
- substring() as string method

### Phase 5: Test (2 hours)
- Verify all stdlib works
- Run test suite
- Fix issues

**Total: 12 hours**

## After Syscalls

**Remove from builtins:**
- assert() → std/test.wyn
- exit() → std/os.wyn
- args() → std/os.wyn
- int_to_str() → std/string.wyn
- system() → std/os.wyn
- write_file() → std/io.wyn
- substring() → string method
- ord() → std/string.wyn

**Only print() remains.**

## Testing Strategy

1. Test each syscall individually
2. Test each stdlib function
3. Run full test suite
4. Verify all 64 tests pass

## Success Criteria

- [ ] syscall() primitive works
- [ ] std/os.wyn pure Wyn
- [ ] std/io.wyn pure Wyn
- [ ] std/string.wyn pure Wyn
- [ ] All tests pass
- [ ] Only print() is builtin
