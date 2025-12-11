# Archived Features

This directory contains features and tests that are planned for future development but not yet fully implemented.

## Structure

- `std/` - 24 standard library modules (stubs or incomplete)
- `examples/` - 29 examples for unimplemented features
- `tests/` - 54 tests for archived features

## Stdlib Modules (24)

### Phase 1: Essential (High Priority)
- `io.wyn` - File I/O operations
- `os.wyn` - Operating system interface
- `time.wyn` - Time and date operations
- `random.wyn` - Random number generation

### Phase 2: Data Structures (Medium Priority)
- `collections.wyn` - HashMap, HashSet, etc.
- `json.wyn` - JSON parsing and serialization
- `regex.wyn` - Regular expressions
- `bits.wyn` - Bit manipulation

### Phase 3: Networking & Security (Medium Priority)
- `net.wyn` - Network operations
- `http.wyn` - HTTP client/server
- `crypto.wyn` - Cryptographic functions
- `hash.wyn` - Hashing algorithms
- `tls.wyn` - TLS/SSL support

### Phase 4: Specialized (Low Priority)
- `gui.wyn` - GUI operations
- `mobile.wyn` - Mobile platform support
- `gpu.wyn` - GPU computing
- `nn.wyn` - Neural networks
- `tui.wyn` - Terminal UI
- `compress.wyn` - Compression
- `encoding.wyn` - Encoding/decoding
- `log.wyn` - Logging
- `utils.wyn` - Utilities
- `validate.wyn` - Validation
- `error.wyn` - Error handling
- `concurrent.wyn` - Advanced concurrency
- `test.wyn` - Testing framework

## Language Features to Implement

### High Priority
- Default parameters
- Result/Option types
- In operator for arrays

### Medium Priority
- Lambda expressions
- Generics
- String interpolation
- Destructuring

### Low Priority
- Traits/interfaces
- Macros
- Compile-time execution

## How to Use

To implement a feature:

1. Move file from `archive/` to main tree
2. Implement functionality
3. Run `make test` to verify
4. Ensure 100% pass rate maintained
5. Commit changes

## Examples

```bash
# Implement io module
mv archive/std/io.wyn std/
# ... implement functions ...
make test  # Verify still 100%

# Add back a test
mv archive/tests/io_test.wyn tests/
# ... fix any issues ...
make test  # Verify still 100%
```

## Notes

- All archived items were working at some point but depend on unimplemented features
- Tests in archive/ may need updates when features are implemented
- Stdlib modules in archive/ are mostly stubs or incomplete implementations
- Examples in archive/ demonstrate features not yet supported
