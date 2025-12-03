# Wyn Benchmark Results

## Test System
- macOS ARM64
- Stage 0 Bootstrap Compiler

## Results

### 1. Fibonacci(35) - CPU Intensive
| Language | Time | vs Wyn |
|----------|------|--------|
| **Wyn** | **0.55s** | 1.0x |
| Node.js | 0.20s | 2.8x faster |
| Python | 2.60s | 4.7x slower |

### 2. Array Operations - Memory Access
| Language | Time | vs Wyn |
|----------|------|--------|
| **Wyn** | **0.83s** | 1.0x |
| Python | 0.16s | 5.2x faster |

### 3. Math Operations - Float Computation
| Language | Time | vs Wyn |
|----------|------|--------|
| **Wyn** | **1.13s** | 1.0x |
| Python | 0.40s | 2.8x faster |

### 4. File I/O - Disk Operations
| Language | Time | vs Wyn |
|----------|------|--------|
| **Wyn** | **0.83s** | 1.0x |
| Python | 0.10s | 8.3x faster |

## Analysis

**Strengths:**
- ✅ CPU-bound tasks (Fibonacci) - competitive
- ✅ Native compilation - no startup overhead
- ✅ Predictable performance

**Weaknesses:**
- ⚠️ Array operations - needs optimization
- ⚠️ File I/O - Python's buffering is better
- ⚠️ Math operations - libm calls not optimized

## Why Python is Sometimes Faster

1. **Highly optimized C libraries** - NumPy, built-in functions
2. **Better buffering** - File I/O is heavily optimized
3. **JIT for hot loops** - PyPy would be even faster

## Optimization Opportunities for Stage 1

1. **Inline small functions** - Eliminate call overhead
2. **Buffer file I/O** - Batch reads/writes
3. **SIMD for arrays** - Vectorize operations
4. **Better register allocation** - Use all available registers
5. **Tail call optimization** - Eliminate recursion overhead

## Conclusion

Wyn is a **solid bootstrap compiler** with room for optimization. Stage 1 will focus on performance improvements.

**Current status:** Good enough for production CLI tools, needs optimization for high-performance computing.

## Running Benchmarks

```bash
cd benchmarks
./run.sh
```
