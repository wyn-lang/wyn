# Pure Wyn Syscall Implementation

Successfully implemented pure Wyn stdlib modules using direct syscalls instead of C functions.

## Modules Created

### 1. std/os_syscall.wyn
Pure Wyn OS operations using syscalls:
- `exit(code)` - Exit process with code
- `getpid()` - Get process ID

**Implementation:**
- Uses macOS ARM64 syscall numbers (base + 0x2000000)
- exit: syscall 33554433 (1 + 0x2000000)
- getpid: syscall 33554452 (20 + 0x2000000)

**Test:** `test_os_syscall.wyn` ✅ PASSED

### 2. std/io_syscall.wyn
Pure Wyn I/O operations using syscalls:
- `write_file(path, content)` - Write string to file

**Implementation:**
- Uses macOS ARM64 syscall numbers
- open: syscall 33554437 (5 + 0x2000000)
- write: syscall 33554436 (4 + 0x2000000)
- close: syscall 33554438 (6 + 0x2000000)
- Flags: O_WRONLY | O_CREAT | O_TRUNC = 1537
- Mode: 0644 = 420 (decimal)

**Test:** `test_io_syscall.wyn` ✅ PASSED
- Successfully writes file with pure syscalls
- Verified output: "Hello from pure Wyn syscalls!"

### 3. std/string_syscall.wyn
Pure Wyn string utilities (no syscalls needed):
- `digit_to_char(digit)` - Convert 0-9 to "0"-"9"
- `char_to_digit(c)` - Convert "0"-"9" to 0-9

**Implementation:**
- Pure Wyn logic using if statements
- No C dependencies, no syscalls needed

**Test:** `test_string_syscall.wyn` ✅ PASSED

**Note:** Full `from_int()` implementation requires string concatenation in loops, which has issues in stage0. Will be implemented when stage1 is self-hosting.

## Test Results

All tests passed successfully:

```
=== OS Syscalls ===
✓ getpid() works - got valid PID
✓ All OS syscall tests passed!

=== I/O Syscalls ===
✓ write_file() works - wrote bytes successfully!
✓ All I/O syscall tests passed!

=== String Functions ===
✓ digit_to_char and char_to_digit work correctly
✓ All string conversion tests passed!
```

## Architecture

The syscall() primitive is implemented in stage0 compiler:
- Generates ARM64 `svc #0` instruction
- Moves syscall number to x16
- Passes arguments in x0-x5
- Returns result in x0

## Platform Notes

**macOS ARM64:**
- Syscall numbers require 0x2000000 offset
- This is a macOS-specific requirement
- Linux ARM64 uses different syscall numbers

## Next Steps

1. ✅ OS syscalls (exit, getpid)
2. ✅ I/O syscalls (write_file)
3. ✅ String utilities (digit conversion)
4. 🔄 Full from_int() - blocked by stage0 limitations
5. 📋 Add read_file() syscall
6. 📋 Port to Linux ARM64 (different syscall numbers)
7. 📋 Move builtins to use these modules

## Impact on Builtin Reduction

These modules demonstrate that we can replace C-based builtins with pure Wyn syscalls:

**Current builtins that can be replaced:**
- `exit()` → `os_syscall.exit()`
- `write_file()` → `io_syscall.write_file()`
- `int_to_str()` → `string_syscall.from_int()` (when stage1 ready)

**Path to 1 builtin:**
Once stage1 is self-hosting, we can move all builtins except `print()` to stdlib modules using syscalls.
