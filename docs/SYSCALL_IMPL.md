# Syscall Implementation Plan

## Goal

Add syscall() primitive to Stage 1, enabling pure Wyn stdlib.

## Step 1: Add syscall() to Stage 1 Codegen

Stage 1 needs to recognize `syscall()` and generate ARM64:

```wyn
# In Stage 1 compiler
fn generate_syscall_code(f: str) {
    # syscall(num, arg1, arg2, arg3, arg4, arg5, arg6)
    # ARM64 syscall convention:
    # x16 = syscall number
    # x0-x5 = arguments
    # svc #0 = make syscall
    
    io.append_file(f, "    mov x16, x0\n")  # syscall number
    io.append_file(f, "    mov x0, x1\n")   # arg1
    io.append_file(f, "    mov x1, x2\n")   # arg2
    io.append_file(f, "    mov x2, x3\n")   # arg3
    io.append_file(f, "    mov x3, x4\n")   # arg4
    io.append_file(f, "    mov x4, x5\n")   # arg5
    io.append_file(f, "    mov x5, x6\n")   # arg6
    io.append_file(f, "    svc #0\n")       # make syscall
}
```

## Step 2: Test syscall()

```wyn
fn main() {
    # SYS_EXIT = 1
    syscall(1, 0, 0, 0, 0, 0, 0)  # exit(0)
}
```

## Step 3: Rewrite std/os.wyn

```wyn
# std/os.wyn - Pure Wyn

fn exit(code: int) {
    syscall(1, code, 0, 0, 0, 0, 0)
}

fn system(cmd: str) -> int {
    # macOS system() syscall
    return syscall(0x2000000 + 59, cmd, 0, 0, 0, 0, 0)
}
```

## Step 4: Rewrite std/io.wyn

```wyn
# std/io.wyn - Pure Wyn

fn write_file(path: str, content: str) -> bool {
    # SYS_OPEN = 5, SYS_WRITE = 4, SYS_CLOSE = 6
    const fd: int = syscall(5, path, 0x601, 0644, 0, 0, 0)
    if fd < 0 {
        return false
    }
    syscall(4, fd, content, content.len(), 0, 0, 0)
    syscall(6, fd, 0, 0, 0, 0, 0)
    return true
}

fn read_file(path: str) -> str {
    const fd: int = syscall(5, path, 0, 0, 0, 0, 0)
    if fd < 0 {
        return ""
    }
    # Allocate buffer (need malloc or fixed buffer)
    const buf: str = alloc(4096)
    syscall(3, fd, buf, 4096, 0, 0, 0)
    syscall(6, fd, 0, 0, 0, 0, 0)
    return buf
}
```

## Step 5: Test Pure Wyn Stdlib

```wyn
import io
import os

fn main() {
    io.write_file("test.txt", "hello")
    const data: str = io.read_file("test.txt")
    print(data)
    os.exit(0)
}
```

## Step 6: Remove Builtins

Once stdlib works with syscalls:
- Remove assert, exit, args, int_to_str, system, write_file, substring, ord from builtins
- Only print() remains

## macOS ARM64 Syscall Numbers

```
SYS_EXIT = 1
SYS_READ = 3
SYS_WRITE = 4
SYS_OPEN = 5
SYS_CLOSE = 6
```

## Timeline

- Add syscall() to Stage 1: 2 hours
- Test syscall(): 1 hour
- Rewrite std/os.wyn: 2 hours
- Rewrite std/io.wyn: 3 hours
- Test pure Wyn: 2 hours
- Remove builtins: 2 hours

**Total: 12 hours**

## Implementation

Starting with adding syscall() recognition to Stage 1 compiler...
