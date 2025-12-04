# Wyn Documentation

## Getting Started
- [Getting Started Guide](1-getting-started.md) - Installation and first program
- [Language Specification](2-language-spec.md) - Complete language reference
- [Package Manager](3-package-manager.md) - Using the wyn tool
- [Standard Library](4-stdlib-reference.md) - Stdlib API reference

## Stage 1 Compiler
- [Type Checker](stage1-type-checker.md) - Enhanced type checking with inference
- [Optimizations](stage1-optimizations.md) - Constant folding, dead code elimination
- [Function Inlining](stage1-inlining.md) - Automatic function inlining (28% faster)

## Concurrency
- [Concurrency Design](concurrency-design.md) - Simple auto-sync model
- [spawn Implementation](spawn-implementation.md) - pthread-based concurrency

## Performance
- [Benchmarks](../benchmarks/FINAL_RESULTS.md) - vs C, Go, Java, Node.js, Python
- [Startup Time](../benchmarks/STARTUP_RESULTS.md) - 2nd fastest (1.29ms)
- [Concurrency](../benchmarks/CONCURRENCY_RESULTS.md) - Matches Go

## Quick Links
- [Examples](../examples/) - Code examples
- [Tests](../tests/) - Test suite
- [Benchmarks](../benchmarks/) - Performance tests
- [GitHub](https://github.com/wyn-lang/wyn) - Source code
