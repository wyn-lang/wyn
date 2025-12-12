# Parallel Test Runner

## Usage

```bash
make test-parallel          # Run with default parallelism (8 jobs)
JOBS=4 make test-parallel   # Run with 4 parallel jobs
```

## Performance

- **Sequential**: ~2m18s (all tests pass)
- **Parallel (8 jobs)**: ~18s (some tests may fail due to race conditions)

## Current Limitations

The parallel test runner is **experimental** and has known issues:

1. **Race Conditions**: Some tests fail when run in parallel due to:
   - Shared runtime state
   - File system conflicts
   - Timing-sensitive tests (spawn tests, time tests)

2. **Not Recommended for CI**: Use `make test` for reliable results

3. **Good for Development**: Useful for quick smoke tests during development

## Why Some Tests Fail

Tests that commonly fail in parallel:
- `spawn_*_test.wyn` - Timing-sensitive concurrency tests
- `time_test.wyn` - Timing-sensitive
- Tests that create/delete files in shared locations

## Future Improvements

To make parallel testing fully reliable:

1. **Test Isolation**: Each test needs its own working directory
2. **Runtime Isolation**: Tests need separate runtime instances
3. **Resource Locking**: Tests that use shared resources need locks
4. **Test Markers**: Mark tests as "parallel-safe" or "sequential-only"

## Recommendation

- **Development**: Use `make test-parallel` for quick feedback
- **CI/Release**: Use `make test` for reliable results
- **Debugging**: Use `make test` to avoid race condition confusion
