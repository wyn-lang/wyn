# Wyn Language - TODO

## Current Status: Tier 1 Complete ✅

### Tier 1: Essential (COMPLETE)
1. ✅ io module - 13 functions (file ops, directory ops)
2. ✅ os module - 8 functions (env, process, filesystem)
3. ✅ net module - 13 functions (HTTP, TCP, UDP, DNS)
4. ✅ json module - 5 functions (parse, stringify, getters)
5. ⏳ Result types - Pending (requires generics implementation)

**Total: 39 infrastructure functions, ALL REAL**

### Tier 2: Important (COMPLETE ✅)
6. ✅ time module - Timestamps, durations, scheduling (7 functions)
7. ✅ log module - Structured logging (5 functions)
8. ✅ encoding module - Base64, hex, URL encoding (6 functions)
9. ✅ hash module - SHA256, MD5, SHA1 for checksums (3 functions)
10. ⏳ http module - HTTP server (pending)

### Language Features (PARALLEL WORK)
- ✅ **String array indexing** - COMPLETE
  - Can create ["a", "b"] and get .len()
  - Can access elements arr[0], arr[-1]
  - Can store in variables: const s: str = arr[0]
  - Unlocked: os.args(), str_split usage, many stdlib functions
- ⏳ Result/Option types (requires generics)
- ⏳ Generics (major feature)
- ⏳ Default parameters
- ⏳ Lambda expressions

### Tier 3: Nice to Have
11. ✅ compress module - gzip, tar for artifacts (COMPLETE)
12. ⏳ crypto module - TLS, certificates
13. ⏳ collections module - HashMap, Set for data processing
14. ✅ regex module - Log parsing, text processing (COMPLETE)

### Developer Experience:
- ⏳ Better error messages
- ⏳ Package manager (wyn pkg)
- ⏳ LSP server for IDE support
- ⏳ Documentation generator
- ⏳ Benchmarking tools

## Test Suite Status
- **Current**: 106/106 passing (100%)
- **Examples**: 82/82
- **Tests**: 24/24

## Work Distribution
- **Agent**: Tier 2 modules (time fix, log, encoding, hash) - COMPLETE ✅
- **Main**: String array indexing ✅, compress ✅, regex ✅
- **Result**: Parallel work successful, no conflicts

## Recent Achievements
- ✅ Implemented all Tier 1 modules (io, os, json, net) - 39 functions
- ✅ Implemented all Tier 2 modules (time, log, encoding, hash) - 21 functions
- ✅ String array indexing complete
- ✅ Tier 3: compress (7 functions) and regex (4 functions) complete
- ✅ Maintained 100% test pass rate throughout
- ✅ Total: 71 infrastructure functions, ALL REAL

## Next Steps
1. ✅ Complete string array indexing (main thread)
2. 🔄 Complete Tier 2 modules (agent)
3. ⏳ Implement HTTP server functionality
4. ⏳ Add Result/Option types (simple version without generics)
5. ⏳ Begin Tier 3 modules

## Notes
- All implementations use real syscalls/libraries, no stubs
- HTTP uses curl via popen
- TCP/UDP use BSD sockets
- JSON uses simple but functional parsing
- Focus on infrastructure/DevOps use cases
