# Standard Library Improvements - 2025-11-30 v7

## Summary

Extended the Wyn standard library with 60+ new utility functions across 4 modules, leveraging the new `.to_float()` method for int-to-float conversions.

## New Module: std/utils.wyn (221 lines)

General-purpose utility functions:

### Numeric Utilities
- `clamp(value, min, max)` - Clamp integer to range
- `clamp_float(value, min, max)` - Clamp float to range
- `swap(a, b)` - Swap two integers
- `is_even(n)`, `is_odd(n)` - Parity checks
- `sign(n)`, `sign_float(n)` - Sign functions (-1, 0, 1)

### Interpolation & Mapping
- `lerp(a, b, t)` - Linear interpolation
- `map_range(value, from_min, from_max, to_min, to_max)` - Map value between ranges

### Number Theory
- `gcd(a, b)` - Greatest common divisor
- `lcm(a, b)` - Least common multiple
- `factorial(n)` - Factorial calculation
- `fibonacci(n)` - Fibonacci number at position n
- `is_prime(n)` - Prime number check

### Digit Operations
- `digit_count(n)` - Count digits in integer
- `reverse_digits(n)` - Reverse digits
- `digit_sum(n)` - Sum of digits
- `is_palindrome(n)` - Check if number is palindrome

### Geometry & Angles
- `deg_to_rad(degrees)` - Degrees to radians
- `rad_to_deg(radians)` - Radians to degrees
- `distance_2d(x1, y1, x2, y2)` - 2D Euclidean distance

### Percentage
- `percentage(value, total)` - Calculate percentage
- `apply_percentage(value, percent)` - Apply percentage to value

## Extended: std/collections.wyn (+159 lines)

Array utility functions:

### Search & Query
- `array_index_of(arr, target)` - Find first occurrence
- `array_last_index_of(arr, target)` - Find last occurrence
- `array_count(arr, target)` - Count occurrences
- `array_contains(arr, target)` - Check if element exists

### Access
- `array_first(arr)` - Get first element (optional)
- `array_last(arr)` - Get last element (optional)

### Slicing
- `array_take(arr, n)` - Take first n elements
- `array_skip(arr, n)` - Skip first n elements

### Chunking
- `array_chunk_count(arr, chunk_size)` - Calculate number of chunks
- `array_get_chunk(arr, chunk_index, chunk_size)` - Get specific chunk

### Transformation
- `array_rotate_left(arr, n)` - Rotate array left
- `array_rotate_right(arr, n)` - Rotate array right
- `array_interleave(a, b)` - Interleave two arrays

## Extended: std/string.wyn (+188 lines)

String manipulation functions:

### Search
- `count_substr(s, substr)` - Count substring occurrences
- `index_of(s, substr)` - Find first occurrence
- `last_index_of(s, substr)` - Find last occurrence

### Formatting
- `truncate(s, max_len, suffix)` - Truncate with suffix
- `center(s, width, pad)` - Center string with padding

### Validation
- `is_numeric(s)` - Check if all digits
- `is_alpha(s)` - Check if all letters
- `is_alphanumeric(s)` - Check if alphanumeric

### Transformation
- `reverse(s)` - Reverse string
- `is_palindrome_str(s)` - Check if palindrome
- `swap_case(s)` - Swap upper/lower case
- `title_case(s)` - Capitalize first letter of each word

## Extended: std/math.wyn (+119 lines)

Float conversion and statistical functions using `.to_float()`:

### Float Rounding
- `round_nearest(x)` - Round to nearest integer
- `floor_float(x)` - Round down
- `ceil_float(x)` - Round up
- `trunc(x)` - Truncate toward zero
- `fract(x)` - Get fractional part

### Statistics
- `array_average(arr)` - Average as float
- `array_median(sorted_arr)` - Median value
- `array_std_dev(arr)` - Standard deviation
- `array_variance(arr)` - Variance

### Helpers
- `float_to_int(x)` - Manual float-to-int conversion

## Testing

All 31 tests pass:
```bash
make test
# Passed: 31
# Failed: 0
```

## Commits

1. `1ad5950` - Add std/utils.wyn with general utility functions
2. `417c67f` - Extend std/collections.wyn with array utilities
3. `0c0ba4e` - Extend std/string.wyn with string utilities
4. `a59afd6` - Extend std/math.wyn with float conversion utilities

## Total Impact

- **New files**: 1 (utils.wyn)
- **Modified files**: 3 (collections.wyn, string.wyn, math.wyn)
- **New functions**: 60+
- **Lines added**: 687
- **All tests passing**: ✓
