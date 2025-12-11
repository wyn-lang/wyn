# Wyn Programming Language

Fast, compiled systems language with clean, expressive syntax, powered by LLVM.

## What is Wyn?

Wyn is a **modern systems language** that combines:
- **Speed:** LLVM-powered native code generation
- **Simplicity:** Clean, expressive syntax
- **Multi-platform:** ARM64, x86_64, and more via LLVM
- **Concurrency:** Go-style green threads (1.3M tasks/sec)
- **Power:** Pattern matching, structs, enums, modules

## 🎉 Production Ready

The compiler is **production-ready** with **100% passing test suite**:
- ✅ Variables (let = mutable, const = immutable)
- ✅ Control flow (if/else, while, for, break, continue)
- ✅ Functions with parameters and recursion
- ✅ Arrays with indexing (including negative indices)
- ✅ Floats and strings
- ✅ Structs and enums
- ✅ Match statements (pattern matching)
- ✅ Spawn blocks (concurrent tasks)
- ✅ Async/await (basic)
- ✅ Module system (5 core stdlib modules)
- ✅ M:N Scheduler (1.3M tasks/sec)

## Test Suite Status

- **Examples**: 76/76 passing (100%)
- **Tests**: 16/16 passing (100%)
- **Total**: 92/92 passing (100%)

## Quick Start

```bash
# 1. Build compiler
make

# 2. Run tests (100% pass)
make test

# 3. Write program
cat > hello.wyn << 'EOF'
fn main() {
    print("Hello, Wyn!")
}
