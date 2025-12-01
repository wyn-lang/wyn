# Wyn Standard Library

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `math` | Math functions (abs, sqrt, pow, gcd, lcm, etc.) | ✅ Working |
| `string` | String utilities (trim, split, pad, case conversion) | ✅ Working |
| `collections` | Array helpers (sum, max, min, contains, slice) | ✅ Working |
| `bits` | Bitwise operations | ✅ Working |
| `hash` | Hash functions (djb2, fnv1a) | ✅ Working |
| `fs` | Path utilities (join, dirname, basename, extension) | ✅ Working |
| `io` | Formatting (format_int, format_hex, format_bin) | ⚠️ Partial |
| `os` | CLI parsing (get_arg, has_flag, getenv) | ⚠️ Partial |
| `time` | Duration helpers, date utilities | ⚠️ Partial |
| `net` | URL parsing, IP utilities | ⚠️ Stubs |
| `http` | HTTP helpers (status codes, headers) | ⚠️ Stubs |
| `json` | JSON utilities | ⚠️ Stubs |
| `crypto` | Hash functions | ⚠️ Stubs |
| `regex` | Character class helpers | ⚠️ Stubs |
| `log` | Logging utilities | ✅ Working |
| `test` | Test assertions | ✅ Working |
| `gui` | GUI framework | ❌ Stubs |
| `tui` | Terminal UI | ⚠️ Stubs |
| `gpu` | GPU computing, tensors | ❌ Stubs |
| `nn` | Neural network primitives | ❌ Stubs |

## Status Legend

- ✅ Working - Pure Wyn implementations, fully functional
- ⚠️ Partial/Stubs - Some functions work, I/O operations need runtime
- ❌ Stubs - API defined, awaiting implementation

## Usage

```wyn
import math
import string

fn main() {
    let x: int = abs(-5)
    let s: str = trim("  hello  ")
    println(x)
    print_str(s)
}
```

## Notes

- All modules use only Stage 0 compiler features
- I/O operations (file, network) are stubs pending syscall support
- GPU/NN modules require backend implementation
