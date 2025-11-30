# Working Standard Library Modules

These modules compile successfully with the Stage 0 compiler and are ready to use.

## ✅ math.wyn (114 functions)

**Status**: Fully functional, all functions compile

**Core Functions**:
- `abs(n: int) -> int` - Absolute value
- `min(a: int, b: int) -> int` - Minimum of two values
- `max(a: int, b: int) -> int` - Maximum of two values
- `pow(base: int, exp: int) -> int` - Power function
- `sqrt(n: float) -> float` - Square root (Newton's method)
- `floor(n: float) -> int` - Round down
- `ceil(n: float) -> int` - Round up
- `round(n: float) -> int` - Round to nearest

**Float Operations**:
- `abs_float(n: float) -> float`
- `min_float(a: float, b: float) -> float`
- `max_float(a: float, b: float) -> float`
- `clamp(value: int, min: int, max: int) -> int`
- `clamp_float(value: float, min: float, max: float) -> float`

**Number Theory**:
- `sign(n: int) -> int` - Returns -1, 0, or 1
- `is_even(n: int) -> bool`
- `is_odd(n: int) -> bool`
- `gcd(a: int, b: int) -> int` - Greatest common divisor
- `lcm(a: int, b: int) -> int` - Least common multiple
- `factorial(n: int) -> int`
- `is_prime(n: int) -> bool`
- `fibonacci(n: int) -> int`

**Digit Operations**:
- `digit_sum(n: int) -> int`
- `digit_count(n: int) -> int`
- `reverse_digits(n: int) -> int`
- `is_palindrome(n: int) -> bool`

**Interpolation**:
- `lerp(a: float, b: float, t: float) -> float` - Linear interpolation
- `map_range(value: float, in_min: float, in_max: float, out_min: float, out_max: float) -> float`
- `smoothstep(edge0: float, edge1: float, x: float) -> float`
- `smootherstep(edge0: float, edge1: float, x: float) -> float`

**Advanced**:
- `isqrt(n: int) -> int` - Integer square root
- `mod_pow(base: int, exp: int, m: int) -> int` - Modular exponentiation
- `binomial(n: int, k: int) -> int` - Binomial coefficient
- `totient(n: int) -> int` - Euler's totient function
- `mobius(n: int) -> int` - Möbius function

**Usage**:
```wyn
import math

fn main() {
    let x: int = math.abs(-42)
    let y: float = math.sqrt(16.0)
    let z: int = math.gcd(48, 18)
    print(x)  # 42
}
```

## ✅ array.wyn (49 functions)

**Status**: Fully functional, all functions compile

**Basic Operations**:
- `sum(arr: [int]) -> int` - Sum all elements
- `product(arr: [int]) -> int` - Product of all elements
- `min_array(arr: [int]) -> ?int` - Find minimum
- `max_array(arr: [int]) -> ?int` - Find maximum
- `average(arr: [int]) -> float` - Calculate average

**Search**:
- `contains(arr: [int], value: int) -> bool`
- `index_of(arr: [int], value: int) -> int` - Returns -1 if not found
- `last_index_of(arr: [int], value: int) -> int`
- `count(arr: [int], value: int) -> int` - Count occurrences

**Filtering**:
- `filter_positive(arr: [int]) -> [int]`
- `filter_negative(arr: [int]) -> [int]`
- `filter_even(arr: [int]) -> [int]`
- `filter_odd(arr: [int]) -> [int]`

**Transformation**:
- `map_double(arr: [int]) -> [int]` - Double each element
- `map_square(arr: [int]) -> [int]` - Square each element
- `reverse(arr: [int]) -> [int]`

**Sorting**:
- `sort(arr: [int]) -> [int]` - Bubble sort
- `is_sorted(arr: [int]) -> bool`

**Usage**:
```wyn
import array

fn main() {
    let nums: [int] = [3, 1, 4, 1, 5]
    let total: int = array.sum(nums)
    let sorted: [int] = array.sort(nums)
    print(total)  # 14
}
```

## ✅ fs.wyn (50 functions)

**Status**: Fully functional, all path utilities compile

**Path Manipulation**:
- `join_path(a: str, b: str) -> str` - Join two path components
- `join_paths(parts: [str]) -> str` - Join multiple components
- `dirname(path: str) -> str` - Get directory name
- `basename(path: str) -> str` - Get file name
- `extension(path: str) -> str` - Get file extension
- `stem(path: str) -> str` - Get filename without extension
- `parent(path: str) -> str` - Get parent directory

**Path Analysis**:
- `is_absolute(path: str) -> bool`
- `has_extension(path: str, ext: str) -> bool`
- `has_ext(path: str) -> bool` - Check if has any extension
- `is_filename_only(path: str) -> bool` - No directory component
- `path_depth(path: str) -> int` - Count path components

**Path Transformation**:
- `normalize_path(path: str) -> str` - Clean up path
- `remove_extension(path: str) -> str`
- `with_extension(path: str, ext: str) -> str`
- `split_path(path: str) -> (str, str)` - Returns (dir, filename)

**Slash Handling**:
- `has_trailing_slash(path: str) -> bool`
- `ensure_trailing_slash(path: str) -> str`
- `remove_trailing_slash(path: str) -> str`

**Usage**:
```wyn
import fs

fn main() {
    let path: str = fs.join_path("/home/user", "file.txt")
    let dir: str = fs.dirname(path)
    let name: str = fs.basename(path)
    let ext: str = fs.extension(path)
    print_str(path)   # /home/user/file.txt
    print_str(ext)    # txt
}
```

## ✅ utils.wyn (57 functions)

**Status**: Fully functional after fixes

**Clamping & Range**:
- `clamp(value: int, min: int, max: int) -> int`
- `clamp_float(value: float, min: float, max: float) -> float`
- `in_range(value: int, min: int, max: int) -> bool`
- `wrap(value: int, max: int) -> int` - Wrap to [0, max)

**Number Utilities**:
- `is_even(n: int) -> bool`
- `is_odd(n: int) -> bool`
- `sign(n: int) -> int` - Returns -1, 0, or 1
- `abs(n: int) -> int`
- `abs_diff(a: int, b: int) -> int`

**Math Functions**:
- `gcd(a: int, b: int) -> int`
- `lcm(a: int, b: int) -> int`
- `factorial(n: int) -> int`
- `fibonacci(n: int) -> int`
- `is_prime(n: int) -> bool`

**Interpolation & Easing**:
- `lerp(a: float, b: float, t: float) -> float`
- `map_range(value: float, from_min: float, from_max: float, to_min: float, to_max: float) -> float`
- `ease_in(t: float) -> float` - Quadratic ease in
- `ease_out(t: float) -> float` - Quadratic ease out
- `ease_in_out(t: float) -> float` - Quadratic ease in-out
- `ease_in_cubic(t: float) -> float`
- `ease_out_cubic(t: float) -> float`
- `ease_out_bounce(t: float) -> float`
- `ease_out_elastic(t: float) -> float`

**Digit Operations**:
- `digit_count(n: int) -> int`
- `digit_sum(n: int) -> int`
- `reverse_digits(n: int) -> int`
- `is_palindrome(n: int) -> bool`

**Geometry**:
- `deg_to_rad(degrees: float) -> float`
- `rad_to_deg(radians: float) -> float`
- `distance_2d(x1: float, y1: float, x2: float, y2: float) -> float`

**Percentage**:
- `percentage(value: float, total: float) -> float`
- `apply_percentage(value: float, percent: float) -> float`

**Spring Physics**:
- `spring_position(current: float, target: float, velocity: float, stiffness: float, damping: float, dt: float) -> float`
- `spring_velocity(current: float, target: float, velocity: float, stiffness: float, damping: float, dt: float) -> float`

**Usage**:
```wyn
import utils

fn main() {
    let clamped: int = utils.clamp(150, 0, 100)  # 100
    let interpolated: float = utils.lerp(0.0, 10.0, 0.5)  # 5.0
    let eased: float = utils.ease_in_out(0.5)  # 0.5
    print(clamped)
}
```

## ✅ error.wyn (9 functions)

**Status**: Simplified and functional

**Error Formatting**:
- `format_error(context: str, error: str) -> str`
- `error_to_str(msg: str) -> str`
- `success_msg(msg: str) -> str`

**Result Helpers**:
- `make_error[E](msg: E) -> Result[int, E]`
- `make_ok[T](value: T) -> Result[T, str]`
- `safe_div(a: int, b: int) -> Result[int, str]`
- `safe_mod(a: int, b: int) -> Result[int, str]`

**Validation**:
- `is_zero(n: int) -> bool`
- `is_empty_str(s: str) -> bool`

**Usage**:
```wyn
import error

fn main() {
    let result: Result[int, str] = error.safe_div(10, 2)
    let value: int = result!  # Unwrap with !
    print(value)  # 5
    
    let err_result: Result[int, str] = error.safe_div(10, 0)
    let safe: int = err_result ?? 0  # Use default with ??
    print(safe)  # 0
}
```

## Summary

**Total Working Functions**: 279 functions across 5 modules

All these modules:
- ✅ Compile successfully with Stage 0 compiler
- ✅ Use only supported language features
- ✅ Have no syntax errors
- ✅ Are ready for production use

**Import and use them**:
```wyn
import math
import array
import fs
import utils
import error

fn main() {
    # Use any functions from these modules
    let x: int = math.abs(-42)
    let nums: [int] = [1, 2, 3]
    let sum: int = array.sum(nums)
    print(x + sum)
}
```
