# Wyn Language - TODO

## Current Status: Tiers 1 & 2 Complete! 🎉

### Tier 1: Essential (COMPLETE ✅)
1. ✅ io module - 13 functions
2. ✅ os module - 8 functions
3. ✅ net module - 13 functions
4. ✅ json module - 5 functions

**Total: 39 functions, ALL REAL**

### Tier 2: Important (COMPLETE ✅)
6. ✅ time module - 7 functions
7. ✅ log module - 5 functions
8. ✅ encoding module - 6 functions
9. ✅ hash module - 3 functions

**Total: 21 functions, ALL REAL**

### Tier 3: Nice to Have (3/4 COMPLETE)
11. ✅ compress module - 7 functions
12. ⏳ crypto module - Runtime ready, integration pending
13. ✅ collections module - 13 functions (HashMap, Set)
14. ✅ regex module - 4 functions

**Total: 24 functions, ALL REAL**

## Grand Total: 84 Infrastructure Functions, ALL REAL! 🚀

### Test Suite Status
- **Current**: 107/107 passing (100%)
- **Examples**: 82/82
- **Tests**: 25/25

### Language Features
- ✅ String array indexing - COMPLETE
- ⏳ Result/Option types (requires generics)
- ⏳ Generics (major feature)
- ⏳ Default parameters
- ⏳ Lambda expressions

### Next Priorities:
1. **Crypto module integration** - Wire up existing runtime functions
2. **HTTP server** - Add server capabilities to net module
3. **Result types** - Simple version without generics
4. **Better error messages** - Add context and suggestions
5. **Documentation generator** - Auto-docs from code

### Developer Experience:
- ⏳ Better error messages
- ⏳ Package manager (wyn pkg)
- ⏳ LSP server for IDE support
- ⏳ Documentation generator
- ⏳ Benchmarking tools

## What Wyn Can Do Now:

### Infrastructure/DevOps:
✅ File operations (read, write, delete, mkdir, etc.)
✅ Process management (exec, env vars, cwd)
✅ Networking (HTTP, TCP, UDP, DNS)
✅ Data processing (JSON, regex, collections)
✅ Logging (structured logging with levels)
✅ Encoding (base64, hex, URL)
✅ Hashing (SHA256, MD5, SHA1, HMAC)
✅ Compression (gzip, tar)
✅ Time operations (timestamps, formatting, sleep)

### Language Features:
✅ All core language features
✅ String arrays with indexing
✅ Pattern matching
✅ Concurrency (spawn, channels)
✅ All operators (arithmetic, bitwise, comparison, logical)

## Notes
- All 84 functions use real syscalls/libraries, no stubs
- HTTP uses curl via popen
- TCP/UDP use BSD sockets
- Crypto uses openssl commands
- Collections use real hash tables
- Focus on infrastructure/DevOps use cases
- 100% test pass rate maintained throughout development
