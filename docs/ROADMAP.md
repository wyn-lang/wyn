# Wyn Language Roadmap

## 🎯 Immediate Goals

### 1. Reduce to 1 Builtin (Target: print only)

**Current:** 4 builtins
**Target:** 1 builtin

| Builtin | Status | Action |
|---------|--------|--------|
| `print()` | ✅ Keep | Target builtin |
| `assert()` | ⏳ Remove | Rename `test` keyword or use different module |
| `args()` | ⏳ Intrinsic | Make compiler intrinsic, not exposed builtin |
| `syscall()` | ⏳ Intrinsic | Make compiler intrinsic, not exposed builtin |

**Completed:**
- ✅ `ord()` → use `string.ord()`
- ✅ `substring()` → use slice syntax `[start:end]`

### 2. Self-Hosting Stage 1 Compiler

**Current:** ~15% complete
**Target:** Stage 1 compiles itself

**Completed:**
- ✅ Variables (let, const)
- ✅ Expressions (arithmetic)
- ✅ Functions (definitions)
- ✅ Control flow (if, while, for)
- ✅ Imports (io, os, string)
- ✅ Dynamic parsing (loop bounds, var init)
- ✅ Multiple variables with tracking

**Needed:**
- ⏳ Function parameters and return values
- ⏳ Variable references in expressions (`while x < max`)
- ⏳ String variable support
- ⏳ Function call parsing
- ⏳ Complex expression evaluation
- ⏳ Full lexer/parser

---

## ✅ Implemented (v0.1)

- [x] Basic types (int, float, str, bool)
- [x] Arrays and strings with slicing
- [x] Functions and structs
- [x] Control flow (if/else, for, while)
- [x] Spawn-based concurrency
- [x] Module system with imports
- [x] Standard library (io, os, string, time, math, array, etc.)
- [x] Stage 0 compiler (C bootstrap)
- [x] Stage 1 compiler (Wyn, partial)
- [x] ARM64 macOS native code generation
- [x] syscall() primitive for pure Wyn stdlib

---

## 🚧 In Progress (v0.2)

### Builtin Reduction
- [x] Remove ord() builtin
- [x] Remove substring() builtin
- [ ] Remove assert() builtin
- [ ] Make args() compiler intrinsic
- [ ] Make syscall() compiler intrinsic

### Self-Hosting
- [x] Basic Stage 1 compiler
- [x] Dynamic value parsing
- [x] Multiple variable support
- [ ] Function parameters
- [ ] Return statements
- [ ] Full expression parsing

### Type System
- [ ] Optional types (`?T`)
- [ ] Result types (`Result[T, E]`)
- [ ] Type aliases

---

## 📋 Planned (v0.3)

### Generics
- [ ] Generic functions `fn foo[T](x: T)`
- [ ] Generic structs `struct Box[T]`
- [ ] Type constraints

### Interfaces
- [ ] Interface definitions
- [ ] `impl` blocks
- [ ] Trait bounds

### Advanced Concurrency
- [ ] Channels
- [ ] Atomic operations
- [ ] Mutex/RWLock

---

## 🔮 Future (v0.4+)

- Async/await
- GPU computing (Metal/Vulkan)
- WebAssembly target
- LSP server
- Package registry

---

## 🎯 Design Principles

1. **Minimal** - Only `print()` as builtin, everything else via imports
2. **Simple** - Python-like syntax, easy to learn
3. **Fast** - Native ARM64 code, near-C performance
4. **Self-hosting** - Compiler written in Wyn

---

## 📊 Current Status

| Metric | Value |
|--------|-------|
| Builtins | 4 (target: 1) |
| Tests passing | ~65% |
| Stage 1 progress | ~15% |
| Platform | ARM64 macOS |
