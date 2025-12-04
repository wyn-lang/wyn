# Wyn Test Suite

**73 tests passing | 0 failures | 100% stdlib coverage**

## Running Tests

```bash
# Run all tests
bash tests/run_tests.sh

# Run specific test
./build/stage0 -o /tmp/test tests/math_stdlib_test.wyn && /tmp/test
```

## Test Organization

- **23 stdlib tests** - `*_stdlib_test.wyn` - One per stdlib module
- **50 core tests** - `*_test.wyn` - Language features

## Coverage

See [TEST_COVERAGE.md](TEST_COVERAGE.md) for detailed coverage information.

All tests are:
- ✅ Non-blocking (CI-ready)
- ✅ Comprehensive (real functionality testing)
- ✅ Fast (parallel execution)
- ✅ No duplicates
