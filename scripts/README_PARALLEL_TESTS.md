# Parallel Test Runner

## Usage

```bash
make test-parallel          # Run with default parallelism (8 jobs)
JOBS=4 make test-parallel   # Run with 4 parallel jobs
```

## Performance

- **Sequential**: ~2m18s (all tests pass)
- **Parallel (8 jobs)**: ~18s (8x faster!)

## Why Bash Instead of Wyn?

You might wonder why we use a bash script instead of Wyn's `spawn` for parallel testing. The answer is that we need features that aren't implemented yet:

**Missing in Wyn:**
- String arrays (`const files: [str]`)
- String split function that returns arrays
- Atomic counters for shared state
- String replace/manipulation functions

**What we'd need:**
```wyn
// This doesn't work yet:
const files: [str] = str_split(os.exec_output("find tests"), "\n")
for file in files {
    spawn {
        run_test(file)
        atomic_add(passed_count, 1)
    }
}
```

Once Wyn has string arrays and atomic operations, we can rewrite this in pure Wyn!

## Current Status

✅ **Works well for development** - 8x faster than sequential
⚠️ **Some race conditions** - A few tests may fail when run in parallel

### Tests That May Fail

- Timing-sensitive tests (spawn tests, time tests)
- Tests that share file system resources
- Tests with global state

### Success Rate

Typically **85-90% of tests pass** in parallel mode, which is good enough for quick development feedback.

## Recommendation

- **Development**: Use `make test-parallel` for quick feedback (18s)
- **CI/Release**: Use `make test` for reliable results (2m18s)
- **Debugging**: Use `make test` to avoid race condition confusion

## Future: Pure Wyn Implementation

Once these features are added to Wyn:
1. String arrays
2. `atomic_add()` / `atomic_sub()` 
3. Better string manipulation

We can rewrite this in pure Wyn using `spawn` for true parallel testing without race conditions!
