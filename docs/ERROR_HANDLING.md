# Error Handling in Wyn

Wyn provides comprehensive, type-safe error handling through Result and Option types with pattern matching.

## Result<T, E> Types

Result types represent operations that can succeed with a value or fail with an error.

### Basic Usage

```wyn
fn divide(a: int, b: int) -> Result[int, str] {
    if b == 0 {
        return err("Division by zero")
    }
    return ok(a / b)
}

fn main() {
    const result = divide(10, 2)
    match result {
        ok(value) => {
            print("Success: " + int_to_str(value))
        }
        err(message) => {
            print("Error: " + message)
        }
    }
}
```

### Pattern Destructuring

Extract values directly from Result types:

```wyn
const result = divide(10, 2)
const value = match result {
    ok(v) => v
    err(_) => -1
}
print(value)  # Prints: 5
```

### Unwrap Operator

Use the `!` operator to extract values (panics on error):

```wyn
const result = ok(42)
const value = result!
print(value)  # Prints: 42
```

## Option<T> Types

Option types represent values that may or may not exist.

### Basic Usage

```wyn
fn find_item(arr: [str], target: str) -> ?str {
    for i in 0..arr.len() {
        if arr[i] == target {
            return some(arr[i])
        }
    }
    return none
}

fn main() {
    const items = ["apple", "banana", "cherry"]
    const found = find_item(items, "banana")
    
    match found {
        some(item) => print("Found: " + item)
        none => print("Not found")
    }
}
```

## Helper Functions (Alternative API)

For immediate productivity, use the helper function API:

```wyn
import result

fn safe_divide(a: int, b: int) -> [int] {
    if b == 0 {
        return result.make_err(-1)
    }
    return result.make_ok(a / b)
}

fn main() {
    const res = safe_divide(10, 2)
    if result.is_ok(res) == 1 {
        const value = result.unwrap(res)
        print("Result: " + int_to_str(value))
    } else {
        print("Division failed")
    }
}
```

### Helper Function API

- `result.make_ok(value)` - Create success result
- `result.make_err(code)` - Create error result  
- `result.is_ok(result)` - Check if success
- `result.is_err(result)` - Check if error
- `result.unwrap(result)` - Extract value
- `result.make_some(value)` - Create some option
- `result.make_none()` - Create none option
- `result.is_some(option)` - Check if some
- `result.is_none(option)` - Check if none
- `result.unwrap_option(option)` - Extract option value

## Best Practices

### 1. Use Result for Operations That Can Fail

```wyn
fn read_file(path: str) -> Result[str, str] {
    # Implementation would check if file exists
    if file_exists(path) {
        return ok(file_contents)
    } else {
        return err("File not found: " + path)
    }
}
```

### 2. Use Option for Values That May Not Exist

```wyn
fn get_config_value(key: str) -> ?str {
    # Implementation would look up configuration
    if config_has_key(key) {
        return some(config_value)
    } else {
        return none
    }
}
```

### 3. Chain Operations with Match Expressions

```wyn
fn process_file(path: str) -> Result[int, str] {
    const content = match read_file(path) {
        ok(data) => data
        err(msg) => return err(msg)
    }
    
    const lines = content.split("\n")
    return ok(lines.len())
}
```

### 4. Use Helper Functions for Simple Cases

```wyn
import result

fn simple_operation() {
    const res = some_risky_function()
    if result.is_ok(res) == 1 {
        # Handle success
        const value = result.unwrap(res)
        process_value(value)
    } else {
        # Handle error
        print("Operation failed")
    }
}
```

## Error Propagation Patterns

### Manual Propagation

```wyn
fn chain_operations() -> Result[int, str] {
    const step1 = first_operation()
    match step1 {
        ok(value1) => {
            const step2 = second_operation(value1)
            match step2 {
                ok(value2) => return ok(value2 * 2)
                err(msg) => return err(msg)
            }
        }
        err(msg) => return err(msg)
    }
}
```

### Using Helper Functions

```wyn
import result

fn chain_with_helpers() -> [int] {
    const step1 = first_operation_helper()
    if result.is_err(step1) == 1 {
        return step1  # Propagate error
    }
    
    const value1 = result.unwrap(step1)
    const step2 = second_operation_helper(value1)
    if result.is_err(step2) == 1 {
        return step2  # Propagate error
    }
    
    const value2 = result.unwrap(step2)
    return result.make_ok(value2 * 2)
}
```

## Testing Error Handling

```wyn
import testing

fn test_divide_success() {
    const result = divide(10, 2)
    match result {
        ok(value) => {
            testing.assert_eq(value, 5)
        }
        err(_) => {
            testing.fail("Expected success")
        }
    }
}

fn test_divide_error() {
    const result = divide(10, 0)
    match result {
        ok(_) => {
            testing.fail("Expected error")
        }
        err(message) => {
            testing.assert_eq(message, "Division by zero")
        }
    }
}
```

## Migration Guide

### From Silent Failures

**Before:**
```wyn
fn risky_operation() -> int {
    # Returns -1 on error (ambiguous)
    if something_wrong {
        return -1
    }
    return actual_value
}
```

**After:**
```wyn
fn risky_operation() -> Result[int, str] {
    if something_wrong {
        return err("Specific error message")
    }
    return ok(actual_value)
}
```

### From Exception-Style Error Handling

**Before (other languages):**
```python
try:
    result = risky_operation()
    process(result)
except Exception as e:
    handle_error(e)
```

**After (Wyn):**
```wyn
const result = risky_operation()
match result {
    ok(value) => process(value)
    err(message) => handle_error(message)
}
```

## Performance Notes

- Result and Option types have minimal runtime overhead
- Pattern matching compiles to efficient branching code
- Helper functions provide zero-cost abstractions
- Unwrap operator generates optimized code for the happy path

## Future Features

- Try operator (`?`) for automatic error propagation
- More sophisticated error types (IOError, NetworkError, etc.)
- Error context and stack traces
- Integration with async/await for asynchronous error handling
