# Wyn Builtins

## Current Status

**Goal:** Only `print()` as a global builtin.  
**Reality:** ~40 builtins currently accessible.

## Why?

The stdlib modules (string, io, os, etc.) call C runtime functions like `int_to_str`, `ord`, `chr`. These must be accessible during stdlib compilation.

Due to how module loading works (parse → merge → type check), we can't distinguish between "builtin called from stdlib" vs "builtin called from user code" at type-check time.

## Available Builtins

### Always Use These Via Stdlib
```wyn
// ❌ DON'T: Direct builtin access
let s: str = int_to_str(42)

// ✅ DO: Via stdlib module
import string
let s: str = string.from_int(42)
```

### Core (Global)
- `print()` - Print to stdout

### String (Use via `import string`)
- ord, chr, substring, str_split, str_find
- str_concat, str_cmp, char_at, int_to_str
- str_substr, str_to_int

### Math (Use via `import math`)
- abs, sqrtf, int_to_float, float_to_int

### File/IO (Use via `import io`)
- len, read_file, write_file, append_file
- file_exists, file_size, read_stdin_line
- mkdir, rmdir, rename_file

### OS (Use via `import os`)
- getcwd, getpid, args

### Time (Use via `import time`)
- time_now, sleep_ms, sleep

### System
- syscall - Low-level system calls

## Best Practice

**Always use stdlib modules:**
```wyn
import string
import io
import os

fn main() {
    let s: str = string.from_int(42)
    io.write_file("out.txt", s)
    os.exit(0)
}
```

**Never call builtins directly** (even though the compiler allows it).

## Future

To enforce "one correct way":
1. Track function source during module loading
2. Only allow builtins in stdlib context
3. Reject builtin calls in user code

This requires refactoring the module loading and type checking pipeline.

## Conclusion

**Convention over enforcement:** Use stdlib modules for all functionality except `print()`.

The compiler currently allows direct builtin access for technical reasons, but this is considered **bad practice** and may be restricted in future versions.
