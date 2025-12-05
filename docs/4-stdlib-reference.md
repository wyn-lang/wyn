# Wyn Standard Library Reference

Complete reference for working Stage 0 stdlib functions.

## Quick Start

```wyn
import math
import string
import bits
import collections
import fs

fn main() {
    const x: int = abs(-42)
    const s: str = trim("  hello  ")
    print(x)
    print(s)
}
```

---

## math - Mathematical Functions

**Import**: `import math`  
**Status**: ✅ 114 functions, all working

### Basic Operations

#### `abs(n: int) -> int`
Return absolute value of an integer.
```wyn
const x: int = abs(-42)  # 42
const y: int = abs(10)   # 10
```

#### `min(a: int, b: int) -> int`
Return minimum of two integers.
```wyn
const x: int = min(5, 3)  # 3
```

#### `max(a: int, b: int) -> int`
Return maximum of two integers.
```wyn
const x: int = max(5, 3)  # 5
```

#### `pow(base: int, exp: int) -> int`
Raise base to the power of exp (base^exp).
```wyn
const x: int = pow(2, 8)   # 256
const y: int = pow(10, 3)  # 1000
```

#### `sqrt(n: float) -> float`
Calculate square root using Newton's method.
```wyn
const x: float = sqrt(16.0)  # 4.0
const y: float = sqrt(2.0)   # 1.414...
```

#### `floor(n: float) -> int`
Round down to nearest integer.
```wyn
const x: int = floor(3.7)   # 3
const y: int = floor(-2.3)  # -3
```

#### `ceil(n: float) -> int`
Round up to nearest integer.
```wyn
const x: int = ceil(3.2)   # 4
const y: int = ceil(-2.7)  # -2
```

#### `round(n: float) -> int`
Round to nearest integer (0.5 rounds away from zero).
```wyn
const x: int = round(3.5)   # 4
const y: int = round(3.4)   # 3
const z: int = round(-2.5)  # -3
```

### Float Operations

#### `abs_float(n: float) -> float`
Absolute value for floats.
```wyn
const x: float = abs_float(-3.14)  # 3.14
```

#### `min_float(a: float, b: float) -> float`
Minimum of two floats.
```wyn
const x: float = min_float(3.14, 2.71)  # 2.71
```

#### `max_float(a: float, b: float) -> float`
Maximum of two floats.
```wyn
const x: float = max_float(3.14, 2.71)  # 3.14
```

#### `clamp(value: int, min: int, max: int) -> int`
Constrain value to range [min, max].
```wyn
const x: int = clamp(150, 0, 100)  # 100
const y: int = clamp(-10, 0, 100)  # 0
const z: int = clamp(50, 0, 100)   # 50
```

#### `clamp_float(value: float, min: float, max: float) -> float`
Constrain float to range.
```wyn
const x: float = clamp_float(1.5, 0.0, 1.0)  # 1.0
```

### Number Theory

#### `sign(n: int) -> int`
Return -1 for negative, 0 for zero, 1 for positive.
```wyn
const x: int = sign(-42)  # -1
const y: int = sign(0)    # 0
const z: int = sign(42)   # 1
```

#### `is_even(n: int) -> bool`
Check if number is even.
```wyn
if is_even(4) {
    print("4 is even")
}
```

#### `is_odd(n: int) -> bool`
Check if number is odd.
```wyn
if is_odd(5) {
    print("5 is odd")
}
```

#### `gcd(a: int, b: int) -> int`
Greatest common divisor using Euclidean algorithm.
```wyn
const x: int = gcd(48, 18)  # 6
const y: int = gcd(100, 35) # 5
```

#### `lcm(a: int, b: int) -> int`
Least common multiple.
```wyn
const x: int = lcm(12, 18)  # 36
```

#### `factorial(n: int) -> int`
Calculate n! (n factorial).
```wyn
const x: int = factorial(5)  # 120
const y: int = factorial(0)  # 1
```

#### `is_prime(n: int) -> bool`
Check if number is prime.
```wyn
if is_prime(17) {
    print("17 is prime")
}
if not is_prime(15) {
    print("15 is not prime")
}
```

#### `fibonacci(n: int) -> int`
Calculate nth Fibonacci number.
```wyn
const x: int = fibonacci(10)  # 55
const y: int = fibonacci(0)   # 0
const z: int = fibonacci(1)   # 1
```

### Digit Operations

#### `digit_sum(n: int) -> int`
Sum of all digits in a number.
```wyn
const x: int = digit_sum(123)  # 6 (1+2+3)
const y: int = digit_sum(999)  # 27
```

#### `digit_count(n: int) -> int`
Count number of digits.
```wyn
const x: int = digit_count(12345)  # 5
const y: int = digit_count(0)      # 1
```

#### `reverse_digits(n: int) -> int`
Reverse the digits of a number.
```wyn
const x: int = reverse_digits(123)  # 321
const y: int = reverse_digits(1000) # 1
```

#### `is_palindrome(n: int) -> bool`
Check if number reads same forwards and backwards.
```wyn
if is_palindrome(12321) {
    print("palindrome")
}
```

### Interpolation

#### `lerp(a: float, b: float, t: float) -> float`
Linear interpolation between a and b by factor t (0.0 to 1.0).
```wyn
const x: float = lerp(0.0, 10.0, 0.5)  # 5.0
const y: float = lerp(0.0, 10.0, 0.25) # 2.5
const z: float = lerp(10.0, 20.0, 0.0) # 10.0
```

#### `map_range(value: float, in_min: float, in_max: float, out_min: float, out_max: float) -> float`
Map value from one range to another.
```wyn
# Map 5 from [0,10] to [0,100]
const x: float = map_range(5.0, 0.0, 10.0, 0.0, 100.0)  # 50.0
```

#### `smoothstep(edge0: float, edge1: float, x: float) -> float`
Smooth Hermite interpolation between 0 and 1.
```wyn
const x: float = smoothstep(0.0, 1.0, 0.5)  # 0.5
```

#### `smootherstep(edge0: float, edge1: float, x: float) -> float`
Smoother version of smoothstep (Ken Perlin's improved version).
```wyn
const x: float = smootherstep(0.0, 1.0, 0.5)
```

### Advanced Functions

#### `isqrt(n: int) -> int`
Integer square root (floor of sqrt).
```wyn
const x: int = isqrt(16)  # 4
const y: int = isqrt(17)  # 4
```

#### `mod_pow(base: int, exp: int, m: int) -> int`
Modular exponentiation: (base^exp) % m.
```wyn
const x: int = mod_pow(2, 10, 1000)  # 24
```

#### `binomial(n: int, k: int) -> int`
Binomial coefficient "n choose k".
```wyn
const x: int = binomial(5, 2)  # 10
```

#### `totient(n: int) -> int`
Euler's totient function φ(n).
```wyn
const x: int = totient(9)  # 6
```

#### `mobius(n: int) -> int`
Möbius function μ(n).
```wyn
const x: int = mobius(6)  # 1
```

---

## string - String Manipulation

**Import**: `import string`  
**Status**: ✅ 35+ functions working

### Basic Operations

#### `str_len(s: str) -> int`
Get string length (alias for .len()).
```wyn
const len: int = str_len("hello")  # 5
```

#### `str_concat(a: str, b: str) -> str`
Concatenate two strings (alias for +).
```wyn
const s: str = str_concat("hello", " world")  # "hello world"
```

#### `str_eq(a: str, b: str) -> bool`
Compare strings for equality (alias for ==).
```wyn
if str_eq("hello", "hello") {
    print("equal")
}
```

### Whitespace Trimming

#### `trim(s: str) -> str`
Remove leading and trailing whitespace.
```wyn
const s: str = trim("  hello  ")  # "hello"
const t: str = trim("\n\tworld\n")  # "world"
```

#### `trim_start(s: str) -> str`
Remove leading whitespace only.
```wyn
const s: str = trim_start("  hello  ")  # "hello  "
```

#### `trim_end(s: str) -> str`
Remove trailing whitespace only.
```wyn
const s: str = trim_end("  hello  ")  # "  hello"
```

### String Queries

#### `starts_with(s: str, prefix: str) -> bool`
Check if string starts with prefix.
```wyn
if starts_with("hello world", "hello") {
    print("starts with hello")
}
```

#### `ends_with(s: str, suffix: str) -> bool`
Check if string ends with suffix.
```wyn
if ends_with("file.txt", ".txt") {
    print("is a text file")
}
```

#### `contains(s: str, substr: str) -> bool`
Check if string contains substring.
```wyn
if contains("hello world", "world") {
    print("contains world")
}
```

#### `is_empty(s: str) -> bool`
Check if string is empty.
```wyn
if is_empty("") {
    print("empty string")
}
```

### Search Operations

#### `index_of(s: str, substr: str) -> int`
Find first occurrence of substring. Returns -1 if not found.
```wyn
const pos: int = index_of("hello world", "world")  # 6
const not_found: int = index_of("hello", "xyz")    # -1
```

#### `last_index_of(s: str, substr: str) -> int`
Find last occurrence of substring.
```wyn
const pos: int = last_index_of("hello hello", "hello")  # 6
```

#### `count_substr(s: str, substr: str) -> int`
Count occurrences of substring.
```wyn
const n: int = count_substr("hello hello hello", "hello")  # 3
```

### Case Conversion

#### `to_upper(s: str) -> str`
Convert string to uppercase.
```wyn
const s: str = to_upper("hello")  # "HELLO"
```

#### `to_lower(s: str) -> str`
Convert string to lowercase.
```wyn
const s: str = to_lower("HELLO")  # "hello"
```

#### `capitalize(s: str) -> str`
Capitalize first character.
```wyn
const s: str = capitalize("hello")  # "Hello"
```

### Character Operations

#### `is_alpha(c: str) -> bool`
Check if character is alphabetic (a-z, A-Z).
```wyn
if is_alpha("a") {
    print("is letter")
}
```

#### `is_digit(c: str) -> bool`
Check if character is digit (0-9).
```wyn
if is_digit("5") {
    print("is digit")
}
```

#### `is_alphanumeric(c: str) -> bool`
Check if character is letter or digit.
```wyn
if is_alphanumeric("a") {
    print("is alphanumeric")
}
```

#### `is_whitespace(c: str) -> bool`
Check if character is whitespace.
```wyn
if is_whitespace(" ") {
    print("is whitespace")
}
```

### String Building

#### `repeat(s: str, n: int) -> str`
Repeat string n times.
```wyn
const s: str = repeat("ha", 3)  # "hahaha"
const dashes: str = repeat("-", 10)  # "----------"
```

#### `pad_left(s: str, width: int, fill: str) -> str`
Pad string on left to width.
```wyn
const s: str = pad_left("42", 5, "0")  # "00042"
```

#### `pad_right(s: str, width: int, fill: str) -> str`
Pad string on right to width.
```wyn
const s: str = pad_right("hello", 10, " ")  # "hello     "
```

#### `join(parts: [str], separator: str) -> str`
Join array of strings with separator.
```wyn
const parts: [str] = ["hello", "world", "!"]
const s: str = join(parts, " ")  # "hello world !"
```

### String Comparison

#### `compare(a: str, b: str) -> int`
Compare strings lexicographically. Returns -1, 0, or 1.
```wyn
const cmp: int = compare("apple", "banana")  # -1
```

#### `compare_ignore_case(a: str, b: str) -> int`
Case-insensitive comparison.
```wyn
const cmp: int = compare_ignore_case("Hello", "hello")  # 0
```

---

## bits - Bit Manipulation

**Import**: `import bits`  
**Status**: ✅ All functions working

### Bit Testing and Setting

#### `bit_test(n: int, pos: int) -> bool`
Check if bit at position is set.
```wyn
const n: int = 5  # 0b101
if bit_test(n, 0) {
    print("bit 0 is set")
}
if bit_test(n, 2) {
    print("bit 2 is set")
}
```

#### `bit_set(n: int, pos: int) -> int`
Set bit at position to 1.
```wyn
const n: int = 0  # 0b000
const m: int = bit_set(n, 2)  # 0b100 = 4
```

#### `bit_clear(n: int, pos: int) -> int`
Clear bit at position to 0.
```wyn
const n: int = 7  # 0b111
const m: int = bit_clear(n, 1)  # 0b101 = 5
```

#### `bit_toggle(n: int, pos: int) -> int`
Toggle bit at position.
```wyn
const n: int = 5  # 0b101
const m: int = bit_toggle(n, 1)  # 0b111 = 7
```

### Bit Counting

#### `popcount(n: int) -> int`
Count number of set bits (population count).
```wyn
const n: int = 7  # 0b111
const count: int = popcount(n)  # 3
```

#### `trailing_zeros(n: int) -> int`
Count trailing zero bits.
```wyn
const n: int = 8  # 0b1000
const tz: int = trailing_zeros(n)  # 3
```

#### `leading_zeros(n: int) -> int`
Count leading zero bits (32-bit).
```wyn
const n: int = 1
const lz: int = leading_zeros(n)  # 31
```

### Bit Manipulation

#### `reverse_bits(n: int) -> int`
Reverse all bits in 32-bit integer.
```wyn
const n: int = 1  # 0b00000001
const r: int = reverse_bits(n)  # 0b10000000...
```

#### `is_power_of_two(n: int) -> bool`
Check if number is power of two.
```wyn
if is_power_of_two(16) {
    print("16 is power of 2")
}
```

#### `next_power_of_two(n: int) -> int`
Find next power of two >= n.
```wyn
const x: int = next_power_of_two(10)  # 16
const y: int = next_power_of_two(16)  # 16
```

### Bit Rotation

#### `rotate_left(n: int, k: int) -> int`
Rotate bits left by k positions (32-bit).
```wyn
const n: int = 1
const r: int = rotate_left(n, 2)  # 4
```

#### `rotate_right(n: int, k: int) -> int`
Rotate bits right by k positions (32-bit).
```wyn
const n: int = 4
const r: int = rotate_right(n, 2)  # 1
```

### Bit Extraction

#### `extract_bits(n: int, start: int, count: int) -> int`
Extract count bits starting at position start.
```wyn
const n: int = 0b11110000
const bits: int = extract_bits(n, 4, 4)  # 0b1111 = 15
```

#### `set_bits(n: int, start: int, count: int, value: int) -> int`
Set count bits starting at position start to value.
```wyn
const n: int = 0
const m: int = set_bits(n, 0, 4, 15)  # 0b1111 = 15
```

---

## collections - Data Structures

**Import**: `import collections`  
**Status**: ⚠️ Read-only operations work

### Array Utilities (Working)

#### `array_first[T](arr: [T]) -> ?T`
Get first element, returns none if empty.
```wyn
const nums: [int] = [1, 2, 3]
const first: ?int = array_first(nums)
match first {
    some(n) => print(n)  # 1
    none => print("empty")
}
```

#### `array_last[T](arr: [T]) -> ?T`
Get last element, returns none if empty.
```wyn
const nums: [int] = [1, 2, 3]
const last: ?int = array_last(nums)  # some(3)
```

#### `array_contains[T](arr: [T], item: T) -> bool`
Check if array contains item.
```wyn
const nums: [int] = [1, 2, 3]
if array_contains(nums, 2) {
    print("found 2")
}
```

#### `array_index_of[T](arr: [T], item: T) -> int`
Find index of item, returns -1 if not found.
```wyn
const nums: [int] = [1, 2, 3]
const idx: int = array_index_of(nums, 2)  # 1
```

#### `array_count[T](arr: [T], item: T) -> int`
Count occurrences of item.
```wyn
const nums: [int] = [1, 2, 2, 3]
const count: int = array_count(nums, 2)  # 2
```

### Stack Operations (Read-only)

#### `stack_new[T]() -> Stack[T]`
Create new empty stack.
```wyn
const s: Stack[int] = stack_new[int]()
```

#### `stack_peek[T](s: Stack[T]) -> ?T`
Peek at top without removing.
```wyn
const s: Stack[int] = Stack[int] { items: [1, 2, 3] }
const top: ?int = stack_peek(s)  # some(3)
```

#### `stack_is_empty[T](s: Stack[T]) -> bool`
Check if stack is empty.
```wyn
if stack_is_empty(s) {
    print("empty stack")
}
```

#### `stack_size[T](s: Stack[T]) -> int`
Get number of items in stack.
```wyn
const size: int = stack_size(s)
```

### Queue Operations (Read-only)

#### `queue_new[T]() -> Queue[T]`
Create new empty queue.
```wyn
const q: Queue[int] = queue_new[int]()
```

#### `queue_peek[T](q: Queue[T]) -> ?T`
Peek at front without removing.
```wyn
const q: Queue[int] = Queue[int] { items: [1, 2, 3] }
const front: ?int = queue_peek(q)  # some(1)
```

#### `queue_is_empty[T](q: Queue[T]) -> bool`
Check if queue is empty.
```wyn
if queue_is_empty(q) {
    print("empty queue")
}
```

#### `queue_size[T](q: Queue[T]) -> int`
Get number of items in queue.
```wyn
const size: int = queue_size(q)
```

**Note**: Push/pop operations are stubs in Stage 0 (require dynamic arrays).

---

## fs - Filesystem Paths

**Import**: `import fs`  
**Status**: ✅ All path utilities working

### Path Joining

#### `join_path(a: str, b: str) -> str`
Join two path components with separator.
```wyn
const path: str = join_path("/home/user", "file.txt")
# "/home/user/file.txt"
```

#### `join_paths(parts: [str]) -> str`
Join multiple path components.
```wyn
const parts: [str] = ["/home", "user", "docs", "file.txt"]
const path: str = join_paths(parts)
# "/home/user/docs/file.txt"
```

### Path Components

#### `dirname(path: str) -> str`
Get directory portion of path.
```wyn
const dir: str = dirname("/home/user/file.txt")  # "/home/user"
const d2: str = dirname("file.txt")  # "."
```

#### `basename(path: str) -> str`
Get filename portion (last component).
```wyn
const name: str = basename("/home/user/file.txt")  # "file.txt"
```

#### `extension(path: str) -> str`
Get file extension including dot.
```wyn
const ext: str = extension("file.txt")  # ".txt"
const e2: str = extension("archive.tar.gz")  # ".gz"
```

#### `stem(path: str) -> str`
Get filename without extension.
```wyn
const stem: str = stem("file.txt")  # "file"
```

#### `parent(path: str) -> str`
Get parent directory (alias for dirname).
```wyn
const p: str = parent("/home/user/file.txt")  # "/home/user"
```

### Path Analysis

#### `is_absolute(path: str) -> bool`
Check if path is absolute.
```wyn
if is_absolute("/home/user") {
    print("absolute path")
}
if not is_absolute("file.txt") {
    print("relative path")
}
```

#### `has_extension(path: str, ext: str) -> bool`
Check if path has specific extension.
```wyn
if has_extension("file.txt", ".txt") {
    print("is text file")
}
```

#### `has_ext(path: str) -> bool`
Check if path has any extension.
```wyn
if has_ext("file.txt") {
    print("has extension")
}
```

#### `is_filename_only(path: str) -> bool`
Check if path has no directory component.
```wyn
if is_filename_only("file.txt") {
    print("just a filename")
}
```

#### `path_depth(path: str) -> int`
Count path components.
```wyn
const depth: int = path_depth("/home/user/docs")  # 3
```

### Path Transformation

#### `normalize_path(path: str) -> str`
Clean up path (remove duplicate slashes).
```wyn
const clean: str = normalize_path("/home//user///file.txt")
# "/home/user/file.txt"
```

#### `remove_extension(path: str) -> str`
Remove file extension.
```wyn
const path: str = remove_extension("file.txt")  # "file"
```

#### `with_extension(path: str, ext: str) -> str`
Replace extension with new one.
```wyn
const path: str = with_extension("file.txt", ".md")
# "file.md"
```

#### `split_path(path: str) -> (str, str)`
Split into directory and filename.
```wyn
let (dir, file): (str, str) = split_path("/home/user/file.txt")
# dir = "/home/user", file = "file.txt"
```

### Slash Handling

#### `has_trailing_slash(path: str) -> bool`
Check if path ends with slash.
```wyn
if has_trailing_slash("/home/user/") {
    print("has trailing slash")
}
```

#### `ensure_trailing_slash(path: str) -> str`
Add trailing slash if missing.
```wyn
const path: str = ensure_trailing_slash("/home/user")
# "/home/user/"
```

#### `remove_trailing_slash(path: str) -> str`
Remove trailing slash if present.
```wyn
const path: str = remove_trailing_slash("/home/user/")
# "/home/user"
```

**Note**: File I/O operations (read, write, exists, etc.) are stubs in Stage 0.

---

## Complete Example

```wyn
import math
import string
import bits
import collections
import fs

fn main() {
    # Math operations
    const x: int = abs(-42)
    const y: int = gcd(48, 18)
    const z: float = sqrt(16.0)
    print(x)  # 42
    print(y)  # 6
    
    # String operations
    const s: str = trim("  hello world  ")
    const upper: str = to_upper(s)
    print(upper)  # "HELLO WORLD"
    
    if starts_with(s, "hello") {
        print("starts with hello")
    }
    
    # Bit operations
    const n: int = 5  # 0b101
    const bits: int = popcount(n)
    print(bits)  # 2
    
    if is_power_of_two(16) {
        print("16 is power of 2")
    }
    
    # Array operations
    const nums: [int] = [1, 2, 3, 4, 5]
    const first: ?int = array_first(nums)
    match first {
        some(n) => print(n)  # 1
        none => print("empty")
    }
    
    # Path operations
    const path: str = join_path("/home/user", "file.txt")
    const dir: str = dirname(path)
    const name: str = basename(path)
    const ext: str = extension(path)
    
    print(path)  # "/home/user/file.txt"
    print(dir)   # "/home/user"
    print(name)  # "file.txt"
    print(ext)   # ".txt"
}
```

---

## Module Summary

| Module | Functions | Status | Description |
|--------|-----------|--------|-------------|
| **math** | 114 | ✅ All working | Mathematical operations |
| **string** | 35+ | ✅ All working | String manipulation |
| **bits** | 20+ | ✅ All working | Bit manipulation |
| **collections** | 20+ | ⚠️ Read-only | Data structures |
| **fs** | 30+ | ✅ Path utils | Filesystem paths |

**Total**: 200+ working functions

---

## Notes

### Stage 0 Limitations

Some functions are stubs that will be implemented in Stage 1:
- **collections**: Push/pop operations (require dynamic arrays)
- **fs**: File I/O operations (require syscalls)
- **string**: split, replace (require dynamic arrays)

### Import System

Functions are imported into the global namespace:
```wyn
import math

fn main() {
    const x: int = abs(-42)  # Direct call, no prefix
}
```

### Type Safety

All functions are fully type-checked:
```wyn
const x: int = abs(-42)        # ✅ OK
const y: float = sqrt(16.0)    # ✅ OK
const z: int = sqrt(16)        # ❌ Type error
```

---

## See Also

- [SPEC.md](../SPEC.md) - Language specification
- [README.md](../README.md) - Project overview
- [std/WORKING_MODULES.md](../std/WORKING_MODULES.md) - Module status
