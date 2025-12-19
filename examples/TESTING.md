# Testing in Wyn

## Test Runner

Wyn has a built-in test runner for your projects.

### Run All Tests

```bash
wyn test
```

This will:
- Auto-discover all `*_test.wyn` files in `tests/` directory
- Run each test
- Show pass/fail results

### Run Specific Test

```bash
wyn test tests/my_test.wyn
```

## Writing Tests

### Simple Test File

```wyn
// tests/math_test.wyn

fn main() {
    print("=== Math Tests ===")
    
    // Test addition
    const sum = 1 + 1
    assert(sum == 2)
    print("Addition: PASS")
    
    // Test multiplication
    const product = 5 * 6
    assert(product == 30)
    print("Multiplication: PASS")
    
    print("All tests passed!")
}
```

### Using Testing Module

```wyn
// tests/string_test.wyn
import testing

fn main() {
    print("=== String Tests ===")
    
    testing.assert_eq("hello".upper(), "HELLO", "upper should work")
    testing.assert_eq("WORLD".lower(), "world", "lower should work")
    testing.assert_true("hello".contains("ll"), "contains should work")
    
    testing.summary()
}
```

## Test Organization

```
my-project/
├── src/
│   └── main.wyn
├── tests/
│   ├── math_test.wyn
│   ├── string_test.wyn
│   └── array_test.wyn
└── README.md
```

## Running Tests

```bash
# Run all tests
wyn test

# Run specific test
wyn test tests/math_test.wyn

# Run with verbose output
wyn run tests/math_test.wyn
```

## Test Output

```
Discovering tests...
Found 3 test files

  ✅ tests/array_test.wyn
  ✅ tests/math_test.wyn
  ✅ tests/string_test.wyn

Tests: 3 passed, 0 failed
```

## Best Practices

1. **Name tests with `_test.wyn` suffix**
2. **Put tests in `tests/` directory**
3. **Use descriptive test names**
4. **Test edge cases**
5. **Use assertions to validate**

## Example Test

```wyn
// tests/calculator_test.wyn

fn add(a: int, b: int) -> int {
    return a + b
}

fn main() {
    print("=== Calculator Tests ===")
    
    // Test basic addition
    assert(add(1, 1) == 2)
    print("✅ Basic addition")
    
    // Test with negatives
    assert(add(-5, 10) == 5)
    print("✅ Negative numbers")
    
    // Test with zero
    assert(add(0, 42) == 42)
    print("✅ Zero handling")
    
    print("\nAll calculator tests passed!")
}
```

## Summary

- ✅ Auto-discovery of test files
- ✅ Run all tests or specific test
- ✅ Clean pass/fail output
- ✅ Use `assert()` or `testing` module
- ✅ Simple and intuitive

**Testing in Wyn is easy and powerful!**
