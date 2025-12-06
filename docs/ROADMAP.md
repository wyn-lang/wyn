# Wyn Language Roadmap

This document outlines planned features for Wyn. Features are organized by priority and implementation status.

---

## ✅ Implemented (v0.1)

- [x] Basic types (int, float, str, bool)
- [x] Arrays and strings
- [x] Functions and structs
- [x] Control flow (if/else, for, while)
- [x] Spawn-based concurrency
- [x] Module system
- [x] Standard library (io, http, time, os, string, array, math)
- [x] Stage 1 compiler with optimizations
- [x] Cross-platform support (macOS, Linux, Windows)

---

## 🚧 In Progress (v0.2)

### Type System Enhancements
- [ ] Optional types (`?T`)
- [ ] Result types (`Result[T, E]`)
- [ ] Basic enums with values
- [ ] Type aliases

### Error Handling
- [ ] `try/catch` blocks
- [ ] `defer` statements
- [ ] Panic and recover

### Pattern Matching
- [ ] `match` expressions
- [ ] Destructuring in match arms
- [ ] Guard clauses

---

## 📋 Planned (v0.3)

### Generics
- [ ] Generic functions `fn foo[T](x: T)`
- [ ] Generic structs `struct Box[T]`
- [ ] Type constraints `[T: Comparable]`

### Interfaces/Traits
- [ ] Interface definitions
- [ ] `impl` blocks for types
- [ ] Trait bounds on generics

### Advanced Concurrency
- [ ] Channels for communication
- [ ] `parallel` loops
- [ ] Atomic operations
- [ ] Mutex and RWLock

---

## 🔮 Future (v0.4+)

### Async/Await
- [ ] `async` functions
- [ ] `await` expressions
- [ ] Async I/O runtime
- [ ] Async iterators with `yield`

### GPU Computing
- [ ] GPU blocks and functions
- [ ] Tensor types
- [ ] Neural network primitives
- [ ] Automatic differentiation
- [ ] Metal/Vulkan/CUDA backends

### Advanced Features
- [ ] Macros and metaprogramming
- [ ] Compile-time execution
- [ ] Custom allocators
- [ ] Inline assembly
- [ ] Foreign function interface (FFI)

### Tooling
- [ ] Language server protocol (LSP)
- [ ] Debugger integration
- [ ] Profiler
- [ ] Package registry
- [ ] Documentation generator

### Compilation Targets
- [ ] WebAssembly
- [ ] iOS/Android native
- [ ] Embedded systems
- [ ] LLVM backend

---

## 🎯 Design Principles

When adding features, we prioritize:

1. **Simplicity** - Keep the language easy to learn
2. **Performance** - Maintain near-C speed
3. **Practicality** - Solve real problems
4. **Explicitness** - No hidden behavior
5. **Compatibility** - Don't break existing code

---

## 📝 Contributing

Want to help implement a feature? Check the [GitHub issues](https://github.com/wyn-lang/wyn/issues) for:
- Features marked "help wanted"
- RFCs (Request for Comments) for design discussions
- Good first issues for newcomers

---

## 🗓️ Release Schedule

- **v0.1** - Current (Basic language working)
- **v0.2** - Q1 2026 (Error handling, pattern matching)
- **v0.3** - Q2 2026 (Generics, interfaces)
- **v0.4** - Q3 2026 (Async/await)
- **v1.0** - Q4 2026 (Stable API, production ready)

*Note: Dates are estimates and subject to change based on community feedback and development progress.*
