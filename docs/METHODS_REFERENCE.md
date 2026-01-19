# Wyn Methods Quick Reference

## String Methods (27)

### Case Conversion
```wyn
text.upper()        // "HELLO"
text.lower()        // "hello"
text.capitalize()   // "Hello"
text.title()        // "Hello World"
```

### Whitespace
```wyn
text.trim()              // Remove both sides
text.trim_left()         // Remove left
text.trim_right()        // Remove right
text.pad_left(10, " ")   // Pad left to width
text.pad_right(10, " ")  // Pad right to width
```

### Information
```wyn
text.len()        // Length
text.is_empty()   // Check if empty
```

### Search
```wyn
text.contains("sub")      // Contains substring
text.starts_with("pre")   // Starts with prefix
text.ends_with("suf")     // Ends with suffix
text.index_of("sub")      // Find index (-1 if not found)
```

### Transformation
```wyn
text.replace("old", "new")  // Replace all
text.slice(0, 5)            // Extract substring
text.repeat(3)              // Repeat N times
text.reverse()              // Reverse string
text.split(",")             // Split to array
text.lines()                // Split by newlines
text.words()                // Split by whitespace
```

### Conversion
```wyn
text.chars()      // To array of chars
text.to_bytes()   // To array of bytes
```

## Number Methods (39)

### Integer Methods (12)
```wyn
num.to_string()    // Convert to string
num.to_float()     // Convert to float
num.abs()          // Absolute value
num.pow(2)         // Power
num.min(10)        // Minimum
num.max(10)        // Maximum
num.clamp(0, 100)  // Clamp to range
num.is_even()      // Check if even
num.is_odd()       // Check if odd
num.is_positive()  // Check if positive
num.is_negative()  // Check if negative
num.is_zero()      // Check if zero
```

### Float Methods (27)
```wyn
// Conversion
num.to_string()    // To string
num.to_int()       // To integer

// Rounding
num.round()        // Round to nearest
num.floor()        // Round down
num.ceil()         // Round up

// Math
num.abs()          // Absolute value
num.pow(2.0)       // Power
num.sqrt()         // Square root
num.min(5.0)       // Minimum
num.max(5.0)       // Maximum
num.clamp(0.0, 10.0)  // Clamp

// Checks
num.is_nan()       // Is NaN
num.is_infinite()  // Is infinite
num.is_finite()    // Is finite
num.is_positive()  // Is positive
num.is_negative()  // Is negative

// Trigonometry
num.sin()          // Sine
num.cos()          // Cosine
num.tan()          // Tangent

// Logarithms
num.log()          // Natural log
num.exp()          // Exponential
```

## Array Methods (18)

```wyn
// Information
arr.len()          // Length
arr.is_empty()     // Check if empty

// Search
arr.contains(val)  // Contains value
arr.index_of(val)  // Find index (-1 if not found)

// Access
arr.get(index)     // Get element at index
arr.first()        // Get first element (Option)
arr.last()         // Get last element (Option)

// Slicing
arr.take(n)        // Take first n elements
arr.skip(n)        // Skip first n elements
arr.slice(start, end)  // Get slice
arr.concat(other)  // Concatenate arrays

// Mutation (modifies array)
arr.push(val)      // Add to end
arr.pop()          // Remove from end (returns value)
arr.reverse()      // Reverse in place
arr.sort()         // Sort in place
```

## Collection Methods (24)

### HashMap Methods (12)
```wyn
map.insert(key, val)     // Insert key-value pair
map.get(key)             // Get value by key
map.remove(key)          // Remove by key
map.contains(key)        // Check if key exists
map.len()                // Number of entries
map.is_empty()           // Check if empty
map.clear()              // Remove all entries
map.get_or_default(key, default)  // Get with fallback
map.merge(other)         // Merge with another map
```

### HashSet Methods (12)
```wyn
set.insert(val)          // Insert value
set.contains(val)        // Check if contains value
set.remove(val)          // Remove value
set.len()                // Number of elements
set.is_empty()           // Check if empty
set.clear()              // Remove all elements
set.union(other)         // Set union
set.intersection(other)  // Set intersection
set.difference(other)    // Set difference
set.is_subset(other)     // Check if subset
set.is_superset(other)   // Check if superset
set.is_disjoint(other)   // Check if disjoint
```

## Option/Result Methods (18)

### Option Methods (9)
```wyn
opt.is_some()            // Check if has value
opt.is_none()            // Check if no value
opt.unwrap()             // Get value (panics if None)
opt.unwrap_or(default)   // Get value or default
opt.expect(msg)          // Get value with custom panic message
opt.or_else(fn)          // Alternative if None
opt.map(fn)              // Transform value if Some
opt.and_then(fn)         // Chain operations
opt.filter(fn)           // Filter value
```

### Result Methods (9)
```wyn
res.is_ok()              // Check if Ok
res.is_err()             // Check if Err
res.unwrap()             // Get value (panics if Err)
res.unwrap_or(default)   // Get value or default
res.expect(msg)          // Get value with custom panic message
res.map_err(fn)          // Transform error
res.or_else(fn)          // Alternative if Err
res.map(fn)              // Transform Ok value
res.and_then(fn)         // Chain operations
```

## Method Chaining

Methods can be chained for fluent APIs:

```wyn
// String chaining
var result = "  HELLO  ".trim().lower().capitalize();
// Result: "Hello"

// Number chaining
var result = 3.7.floor().to_int();
// Result: 3

// Multiple operations
var text = "hello,world,test"
    .split(",")
    .len();
// Result: 3
```

## Type-Aware Dispatch

Same method name works on different types:

```wyn
var text = "hello";
var text_len = text.len();  // Calls string_len()

var arr = [1, 2, 3];
var arr_len = arr.len();    // Calls array_len()
```

## Common Patterns

### String Processing
```wyn
fn clean_input(text: string) -> string {
    return text.trim().lower();
}
```

### Number Validation
```wyn
fn is_valid_age(age: int) -> bool {
    return age.is_positive() && age.clamp(0, 150) == age;
}
```

### Array Operations
```wyn
fn sort_and_get_first(arr: [int]) -> int {
    arr.sort();
    return arr.get(0);
}
```

### CSV Parsing
```wyn
fn parse_csv_line(line: string) -> int {
    var parts = line.trim().split(",");
    return parts.len();
}
```

## Performance

- Methods compile to the same C code as function calls
- No runtime overhead
- Zero-cost abstraction

## Compatibility

- All old function-style APIs still work
- Fully backward compatible
- Mix old and new styles freely

## See Also

- [README.md](README.md) - Full feature list
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - Migration strategies
- [examples/11_methods.wyn](examples/11_methods.wyn) - Working examples
