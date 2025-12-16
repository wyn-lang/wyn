# Wyn Language - TODO

> **See [VISION.md](VISION.md) for long-term roadmap and strategic direction.**
> **See [STATUS.md](STATUS.md) for current capabilities.**

## Recent Achievements (Dec 16-17, 2025)

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
- **Error Handling System** ✅
  - Result<T, E> and Option<T> types ✅
  - Pattern destructuring with variable binding ✅
  - Match expressions and statements ✅
  - Unwrap operator (!) ✅
  - Helper function API ✅
- File operations (copy, move, chmod, glob)
- CLI argument parsing
- Shebang support
- Inline execution

**Result:** 16 modules, 120+ functions, 35/35 tests passing (100%)

## Immediate Priorities

### 🔥 Critical
1. ~~**Error Handling System**~~ ✅ COMPLETE
   - [x] Result<T, E> type implementation ✅
   - [x] Option<T> type implementation ✅
   - [x] Pattern destructuring with variable binding ✅
   - [x] Match expressions and statements ✅
   - [x] Unwrap operator (!) ✅
   - [x] Helper function API ✅
   - [ ] Try operator (?) - future enhancement
   - [ ] Stdlib integration - future enhancement

2. **Package Ecosystem** (70% complete)
   - [x] Match expression LLVM codegen ✅
   - [ ] Pattern destructuring (ok(v), err(msg), some(v))
   - [ ] Unwrap operator (!) implementation
   - [ ] Try operator (?) implementation
   - [ ] Replace stdlib silent failures with Result returns

2. **Package Ecosystem**
   - [x] Package manager ✅
   - [x] Create 3 official packages (aws, postgres, docker) ✅
   - [ ] Publish packages to GitHub
   - [ ] Import resolution for packages
   - [ ] Package registry/discovery

3. ~~**Complete List Comprehensions**~~ ✅ DONE
   - [x] Parser ✅
   - [x] Codegen ✅

4. ~~**True Method Chaining**~~ ✅ DONE
   - [x] Pseudo-chaining ✅
   - [x] Real `.method().method()` syntax ✅

### 🎯 High Priority
4. **Windows Support** (50% complete - deferred)
   - [x] Add Windows compatibility layer ✅
   - [x] Fix path separators ✅
   - [x] Fix command execution ✅
   - [ ] Test on actual Windows machine
   - [ ] Fix issues found in testing
   - [ ] CI/CD for Windows

5. **Package Ecosystem**
   - [x] Package manager ✅
   - [x] Create 3 official packages (aws, postgres, docker) ✅
   - [ ] Publish packages to GitHub
   - [ ] Import resolution for packages

5. ~~**Generics**~~ ✅ DONE
   - [x] Complete implementation ✅
   - [x] Monomorphization ✅
   - [ ] Result<T, E> type (using Result helpers for now)
   - [ ] Option<T> type (using Option helpers for now)

6. **Advanced Language Features**

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
- [x] Complete error handling system (Result/Option types) ✅
- [x] 10+ real-world examples ✅
- [x] Package manager MVP ✅
- [ ] Package ecosystem with 5+ published packages
- [ ] 1000 GitHub stars

### 6 Months
- [ ] Windows compatibility (when core is more mature)
- [ ] 50+ packages
- [ ] 10+ production deployments
- [ ] Complete documentation
- [ ] 5000 GitHub stars

---

**Progress:** 55% of vision complete (up from 45%)
**Status:** Core is production-ready, error handling complete, ecosystem growing
**Next Focus:** Package ecosystem, advanced language features
