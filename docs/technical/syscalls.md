# Syscalls in Wyn

Wyn provides direct access to system calls via the `syscall()` builtin, enabling pure Wyn implementations of I/O, OS operations, and more without C dependencies.

---

## The syscall() Primitive

```wyn
fn syscall(num: int, arg1: int, arg2: int, arg3: int, arg4: int, arg5: int, arg6: int) -> int
```

Makes a direct system call with up to 6 arguments.

**Parameters:**
- `num` - Syscall number (platform-specific)
- `arg1-arg6` - Arguments to the syscall
- Returns: Result from the syscall (usually 0 on success, negative on error)

**Example:**

```wyn
# exit(0) - syscall number 1 on macOS ARM64
syscall(33554433, 0, 0, 0, 0, 0, 0)
```

---

## How It Works

The compiler generates ARM64 assembly for `syscall()`:

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

This is a compiler intrinsic - the compiler recognizes `syscall()` and generates the appropriate assembly.

---

## macOS ARM64 Syscall Numbers

macOS requires adding `0x2000000` to BSD syscall numbers:

```wyn
# Common syscalls
const SYS_EXIT: int = 33554433      # 1 + 0x2000000
const SYS_READ: int = 33554435      # 3 + 0x2000000
const SYS_WRITE: int = 33554436     # 4 + 0x2000000
const SYS_OPEN: int = 33554437      # 5 + 0x2000000
const SYS_CLOSE: int = 33554438     # 6 + 0x2000000
const SYS_GETPID: int = 33554452    # 20 + 0x2000000
```

**Why 0x2000000?**

macOS uses this offset to distinguish BSD syscalls from Mach syscalls. All BSD syscalls need this offset on macOS ARM64.

---

## Using Syscalls

### Exit Program

```wyn
fn exit(code: int) {
    syscall(33554433, code, 0, 0, 0, 0, 0)
}

fn main() {
    print("Exiting...")
    exit(0)
}
```

### Write to File

```wyn
fn write_file(path: str, content: str) -> int {
    # open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)
    # Flags: O_WRONLY=1, O_CREAT=512, O_TRUNC=1024 = 1537
    # Mode: 0644 = 420 decimal
    const fd: int = syscall(33554437, path, 1537, 420, 0, 0, 0)
    
    if fd < 0 {
        return fd  # Error
    }
    
    # write(fd, content, length)
    const written: int = syscall(33554436, fd, content, content.len(), 0, 0, 0)
    
    # close(fd)
    syscall(33554438, fd, 0, 0, 0, 0, 0)
    
    return written
}
```

### Get Process ID

```wyn
fn getpid() -> int {
    return syscall(33554452, 0, 0, 0, 0, 0, 0)
}

fn main() {
    const pid: int = getpid()
    print("PID:", pid)
}
```

---

## File Open Flags

Common flags for `open()` syscall:

```wyn
const O_RDONLY: int = 0        # Read only
const O_WRONLY: int = 1        # Write only
const O_RDWR: int = 2          # Read and write
const O_CREAT: int = 512       # Create if doesn't exist
const O_TRUNC: int = 1024      # Truncate to zero length
const O_APPEND: int = 8        # Append mode

# Common combinations
const O_WRITE_CREATE: int = 1537   # O_WRONLY | O_CREAT | O_TRUNC
const O_APPEND_CREATE: int = 521   # O_WRONLY | O_CREAT | O_APPEND
```

---

## File Permissions

Unix file permissions (octal → decimal):

```wyn
const MODE_0644: int = 420     # rw-r--r-- (owner read/write, others read)
const MODE_0755: int = 493     # rwxr-xr-x (owner all, others read/execute)
const MODE_0600: int = 384     # rw------- (owner read/write only)
```

---

## Error Handling

Syscalls return negative values on error:

```wyn
const fd: int = syscall(33554437, "file.txt", 0, 0, 0, 0, 0)

if fd < 0 {
    print("Error opening file")
    # Common error codes (negative):
    # -2: ENOENT (file not found)
    # -13: EACCES (permission denied)
    # -17: EEXIST (file exists)
}
```

---

## Standard Library Implementation

The Wyn standard library uses syscalls for all I/O:

### std/os.wyn

```wyn
fn exit(code: int) {
    syscall(33554433, code, 0, 0, 0, 0, 0)
}

fn getpid() -> int {
    return syscall(33554452, 0, 0, 0, 0, 0, 0)
}
```

### std/io.wyn

```wyn
fn write_file(path: str, content: str) -> int {
    const fd: int = syscall(33554437, path, 1537, 420, 0, 0, 0)
    if fd < 0 {
        return fd
    }
    const written: int = syscall(33554436, fd, content, content.len(), 0, 0, 0)
    syscall(33554438, fd, 0, 0, 0, 0, 0)
    return written
}
```

---

## Complete Syscall Reference

### Process Management

| Syscall | Number | Description |
|---------|--------|-------------|
| exit | 33554433 | Exit process |
| fork | 33554434 | Create child process |
| getpid | 33554452 | Get process ID |
| getppid | 33554471 | Get parent process ID |

### File Operations

| Syscall | Number | Description |
|---------|--------|-------------|
| read | 33554435 | Read from file descriptor |
| write | 33554436 | Write to file descriptor |
| open | 33554437 | Open file |
| close | 33554438 | Close file descriptor |
| lseek | 33554471 | Seek in file |
| unlink | 33554442 | Delete file |

### Directory Operations

| Syscall | Number | Description |
|---------|--------|-------------|
| mkdir | 33554568 | Create directory |
| rmdir | 33554569 | Remove directory |
| chdir | 33554444 | Change directory |
| getcwd | 33554458 | Get current directory |

---

## Platform Notes

**Current Support:** macOS ARM64

**Future Platforms:**
- Linux ARM64 (different syscall numbers, no offset)
- Linux x86_64 (different syscall numbers and calling convention)
- Other Unix systems

The stdlib abstracts syscall differences - user code remains portable.

---

## Why Syscalls?

**Benefits:**
1. **No C dependencies** - Pure Wyn implementations
2. **Minimal builtins** - Only `print()` needed for I/O
3. **Full control** - Direct access to OS features
4. **Performance** - No overhead from C wrappers
5. **Transparency** - See exactly what the code does

**Example:**

```wyn
# Before: 30+ builtins
print("Hello")
write_file("test.txt", "data")  # Builtin
exit(0)  # Builtin

# After: 1 builtin + imports
import io
import os

print("Hello")  # Only builtin
io.write_file("test.txt", "data")  # Pure Wyn via syscalls
os.exit(0)  # Pure Wyn via syscalls
```

---

## Learn More

- [Language Reference](language.md)
- [Standard Library](stdlib.md)
- [Quick Start](quickstart.md)
