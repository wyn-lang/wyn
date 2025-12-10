# Experimental Stdlib Implementations

This directory contains **pure Wyn** implementations of stdlib modules using direct syscalls.

## Purpose

These are alternative implementations that:
- Use direct syscalls instead of C builtins
- Demonstrate pure Wyn without C dependencies
- Serve as reference for low-level syscall usage

## Status

⚠️ **Experimental** - Not used by default

- Platform-specific (macOS ARM64 only)
- Less portable than C builtin approach
- Useful for learning and future self-hosting

## Files

- `io_syscall.wyn` - File I/O via syscalls
- `os_syscall.wyn` - OS operations via syscalls  
- `string_syscall.wyn` - String utilities in pure Wyn

## Usage

The main stdlib (`../string.wyn`, `../io.wyn`, etc.) uses C builtins and is production-ready.

These experimental versions can be used by explicitly importing:
```wyn
import experimental.io_syscall
```

## Future

These may become the foundation for a fully self-hosted Wyn compiler with no C runtime dependencies.
