# Wyn Benchmarks

Comprehensive performance testing suite comparing Wyn against C, Go, Java, Node.js, and Python.

## Quick Results

**Wyn is 20-30x faster than Python** and competitive with C and Go.

See [FINAL_RESULTS.md](FINAL_RESULTS.md) for complete analysis.

## Benchmark Categories

### CPU Performance
- **Fibonacci** - Recursive computation
- **Primes** - Prime number generation
- **Math** - Floating point operations

### Memory & Arrays
- **Arrays** - Large array operations
- **Memory** - Allocation and access patterns

### I/O Performance
- **File I/O** - Read/write operations
- **Network** - TCP/HTTP benchmarks

### Startup Time
- **Cold start** - Time to first execution
- See [STARTUP_RESULTS.md](STARTUP_RESULTS.md)

### Concurrency
- **Spawn** - Concurrent task execution
- **Atomic** - Atomic operations
- See [CONCURRENCY_RESULTS.md](CONCURRENCY_RESULTS.md)

## Running Benchmarks

```bash
# Run specific benchmarks manually
cd benchmarks/cpu
../../build/stage0 --stage1-opt -o fib_wyn fib.wyn
time ./fib_wyn

cd ../memory
../../build/stage0 --stage1-opt -o array_wyn array.wyn
time ./array_wyn
```

**Note:** Benchmark automation coming soon in Wyn.

## Test Environment

- **Platform**: macOS ARM64 (Apple Silicon)
- **Wyn**: Stage 1 with optimizations
- **Compiler flags**: `--stage1-tc --stage1-opt`
- **Date**: December 2025

## Key Findings

### Performance Tier
1. **Tier S**: C, Go
2. **Tier A**: **Wyn**, Java ⭐
3. **Tier B**: Node.js
4. **Tier F**: Python

### Wyn Strengths
- ✅ Matches C on array operations
- ✅ 2nd fastest startup (1.29ms)
- ✅ 20-30x faster than Python
- ✅ Competitive with compiled languages

### Use Cases
- High-performance applications
- Systems programming
- CLI tools (fast startup)
- Server applications
- Data processing

## Benchmark Structure

```
benchmarks/
├── README.md              # This file
├── FINAL_RESULTS.md       # Complete analysis
├── STARTUP_RESULTS.md     # Startup benchmarks
├── CONCURRENCY_RESULTS.md # Concurrency tests
├── run.sh                 # Run all benchmarks
├── cpu/                   # CPU benchmarks
├── memory/                # Memory benchmarks
├── io/                    # I/O benchmarks
├── network/               # Network benchmarks
├── startup/               # Startup tests
└── concurrency/           # Concurrency tests
```

## Contributing

To add new benchmarks:
1. Create test in appropriate category directory
2. Implement in all languages (Wyn, C, Go, Java, Node.js, Python)
3. Add to category's run.sh
4. Update results in FINAL_RESULTS.md

## Methodology

- Each benchmark runs 3 times, median reported
- Cold start measurements for startup
- Real-world workloads (not micro-benchmarks)
- Fair comparison (same algorithms across languages)
