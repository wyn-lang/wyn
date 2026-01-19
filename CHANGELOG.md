# Changelog

## [1.1.0] - 2026-01-19

### Added
- **Module System**: Full module system with 11 search paths
  - Public/private visibility with `pub` keyword
  - Nested imports and user module override
  - Built-in math module with 7 functions (sqrt, pow, sin, cos, tan, abs, floor)
- **Cross-platform Support**: Release binaries for Linux (x64, ARM64), macOS (x64, ARM64), and Windows (x64)

### Fixed
- Method call code generation for extension methods, string methods, and number methods
- Built-in math module accessibility (all 7 functions now work)
- Test pollution with proper cleanup of `/tmp/wyn_modules/`
- Linux build compatibility with POSIX defines
- Windows compatibility issues (TokenType conflicts, time_t conversions, dirent.h)
- Compilation paths with WYN_ROOT environment variable

### Changed
- Release workflow now runs tests before cross-compilation
- Test scripts use relative paths for better portability

### Validated
- 21/21 examples pass
- 11/11 module tests pass
- All advertised features verified working
