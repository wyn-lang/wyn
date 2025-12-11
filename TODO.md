# Wyn Language - TODO

## Current Status: Tier 1 Complete ✅

### Tier 1: Essential (COMPLETE)
1. ✅ io module - 13 functions (file ops, directory ops)
2. ✅ os module - 8 functions (env, process, filesystem)
3. ✅ net module - 13 functions (HTTP, TCP, UDP, DNS)
4. ✅ json module - 5 functions (parse, stringify, getters)
5. ⏳ Result types - Pending (requires generics implementation)

**Total: 39 infrastructure functions, ALL REAL**

### Tier 2: Important (IN PROGRESS)
6. 🔄 time module - Timestamps, durations, scheduling (delegated)
7. 🔄 log module - Structured logging (delegated)
8. 🔄 encoding module - Base64, hex, URL encoding (delegated)
9. 🔄 hash module - SHA256, MD5 for checksums (delegated)
10. ⏳ http module - HTTP server (pending)

### Tier 3: Nice to Have
11. ⏳ compress module - gzip, tar for artifacts
12. ⏳ crypto module - TLS, certificates
13. ⏳ collections module - HashMap, Set for data processing
14. ⏳ regex module - Log parsing, text processing

### Language Features Needed:
- ⏳ String array indexing (partial support exists)
- ⏳ Result/Option types (requires generics)
- ⏳ Generics (major feature)
- ⏳ Default parameters
- ⏳ Lambda expressions

### Developer Experience:
- ⏳ Better error messages
- ⏳ Package manager (wyn pkg)
- ⏳ LSP server for IDE support
- ⏳ Documentation generator
- ⏳ Benchmarking tools

## Test Suite Status
- **Current**: 98/98 passing (100%)
- **Examples**: 80/80
- **Tests**: 18/18

## Recent Achievements
- Implemented all Tier 1 modules with real functionality
- Added string array support (creation and length)
- Extended modules with 39 infrastructure functions
- Maintained 100% test pass rate throughout

## Next Steps
1. Complete Tier 2 modules (time, log, encoding, hash)
2. Implement HTTP server functionality
3. Add Result/Option types (simple version without generics)
4. Improve string array support (indexing)
5. Begin Tier 3 modules

## Notes
- All implementations use real syscalls/libraries, no stubs
- HTTP uses curl via popen
- TCP/UDP use BSD sockets
- JSON uses simple but functional parsing
- Focus on infrastructure/DevOps use cases
