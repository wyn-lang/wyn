# Wyn Language - TODO

> **See [VISION.md](VISION.md) for long-term roadmap and strategic direction.**
> **See [STATUS.md](STATUS.md) for current capabilities.**

## Recent Achievements (Dec 15-16, 2025)

### ✅ Completed
- Unified CLI binary
- First-class functions (lambdas)
- **Ternary operator** ✅
- **Type system fixes** (strings/arrays) ✅
- **Array operations** (reverse, append, prepend, contains) ✅
- **String utilities** (upper, lower, trim) ✅
- **Database module** (SQLite) ✅
- **HTTP framework** (routing) ✅
- **Testing framework** ✅
- **Package manager Phase 1** (install, list, update) ✅
- **List comprehensions** ✅
- **Method chaining** ✅
- File operations (copy, move, chmod, glob)
- CLI argument parsing
- Shebang support
- Inline execution

**Result:** 16 modules, 120+ functions, 35/35 tests passing (100%)

## Immediate Priorities

### 🔥 Critical
1. **Windows Support**
   - [ ] Test all functions on Windows
   - [ ] Fix path separators
   - [ ] Fix line endings
   - [ ] CI/CD for Windows

2. ~~**Complete List Comprehensions**~~ ✅ DONE
   - [x] Parser ✅
   - [x] Codegen ✅

3. ~~**True Method Chaining**~~ ✅ DONE
   - [x] Pseudo-chaining ✅
   - [x] Real `.method().method()` syntax ✅

### 🎯 High Priority
4. **Package Ecosystem**
   - [x] Package manager ✅
   - [ ] Create 5-10 official packages
   - [ ] Import resolution for packages

5. **Generics**
   - [ ] Complete implementation
   - [ ] Result<T, E> type
   - [ ] Option<T> type

6. **Error Handling**
   - [ ] Result/Option types
   - [ ] Proper error propagation

## Next Month

### Language Features
- [ ] Destructuring
- [ ] Traits/interfaces
- [ ] Async/await
- [ ] Closures (capture variables)

### Stdlib Additions
- [ ] More database drivers (postgres, mysql)
- [ ] SSH client
- [ ] Git operations
- [ ] Template engine

### Tooling
- [ ] Debugger
- [ ] Profiler
- [ ] Better LSP
- [ ] Documentation generator

## Package Ecosystem

### Official Packages to Create
1. [ ] **aws** - AWS SDK
2. [ ] **postgres** - PostgreSQL driver
3. [ ] **mysql** - MySQL driver
4. [ ] **docker** - Docker API
5. [ ] **k8s** - Kubernetes client
6. [ ] **redis** - Redis client
7. [ ] **http-router** - Advanced routing
8. [ ] **cli-framework** - CLI app framework

## Documentation

- [x] README.md ✅
- [x] STATUS.md ✅
- [x] TODO.md ✅
- [x] VISION.md ✅
- [ ] Tutorial series
- [ ] API reference (complete)
- [ ] Best practices guide
- [ ] Migration guides

## Community

- [ ] Make repository public
- [ ] Create website
- [ ] Write blog post
- [ ] Post to Show HN
- [ ] Create Discord/forum
- [ ] Conference talks

## Success Metrics

### 3 Months
- [ ] 100% Windows compatibility
- [x] 10+ real-world examples ✅
- [x] Package manager MVP ✅
- [ ] 1000 GitHub stars

### 6 Months
- [ ] 50+ packages
- [ ] 10+ production deployments
- [ ] Complete documentation
- [ ] 5000 GitHub stars

---

**Progress:** 45% of vision complete
**Status:** Core is production-ready, ecosystem growing
**Next Focus:** Windows support, complete advanced features
