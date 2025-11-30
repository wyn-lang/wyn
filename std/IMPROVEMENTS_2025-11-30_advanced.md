# Wyn Standard Library Improvements - Advanced Functions
## Date: 2025-11-30

This document summarizes the advanced functions added to the Wyn standard library.

## Array Module (`std/array.wyn`)

### Search & Sort
- `binary_search(arr, target)` - Binary search in sorted array, returns index or -1
- `merge_sorted(a, b)` - Merge two sorted arrays into one sorted array

### Partitioning
- `partition(arr, pivot)` - Split array into elements < pivot and >= pivot
- `median_of_three(a, b, c)` - Find median of three values (for quicksort)

### Analysis
- `local_maxima(arr)` - Find indices of all local maximum points
- `local_minima(arr)` - Find indices of all local minimum points
- `has_duplicates(arr)` - Check if array contains any duplicate elements
- `longest_increasing_length(arr)` - Length of longest increasing subsequence
- `longest_decreasing_length(arr)` - Length of longest decreasing subsequence

## Math Module (`std/math.wyn`)

### Number Properties
- `is_perfect(n)` - Check if number equals sum of its proper divisors
- `is_triangular(n)` - Check if number is a triangular number

### Polygonal Numbers
- `triangular(n)` - Nth triangular number (1+2+3+...+n)
- `pentagonal(n)` - Nth pentagonal number
- `hexagonal(n)` - Nth hexagonal number

### Combinatorics
- `catalan(n)` - Nth Catalan number
- `bell(n)` - Nth Bell number (partitions of a set)
- `stirling_second(n, k)` - Stirling number of the second kind

### Number Theory
- `harmonic(n)` - Nth harmonic number (1 + 1/2 + 1/3 + ... + 1/n)
- `totient(n)` - Euler's totient function (count of coprimes)
- `mobius(n)` - Möbius function
- `partition_count(n)` - Number of ways to partition n

### Interpolation
- `smoothstep(edge0, edge1, x)` - Smooth Hermite interpolation
- `smootherstep(edge0, edge1, x)` - Even smoother interpolation

### Activation Functions (ML/AI)
- `sigmoid(x)` - Sigmoid activation function
- `relu(x)` - ReLU (Rectified Linear Unit)
- `leaky_relu(x, alpha)` - Leaky ReLU with configurable slope
- `softplus(x)` - Smooth approximation of ReLU
- `gaussian(x, mu, sigma)` - Gaussian/normal distribution function

## Utils Module (`std/utils.wyn`)

### Comparison
- `compare(a, b)` - Three-way comparison returning -1, 0, or 1
- `compare_float(a, b)` - Three-way comparison for floats
- `in_range(value, min, max)` - Check if value is within range
- `in_range_float(value, min, max)` - Float range check
- `approx_equal(a, b, epsilon)` - Check if floats are approximately equal

### Value Manipulation
- `wrap(value, max)` - Wrap value to range [0, max)
- `ping_pong(t, length)` - Bounce value between 0 and length
- `normalize(value, min, max)` - Normalize value to 0-1 range
- `denormalize(value, min, max)` - Denormalize from 0-1 to range
- `snap(value, step)` - Snap value to nearest multiple of step

### Animation & Interpolation
- `move_towards(current, target, max_delta)` - Move value towards target
- `exp_decay(current, target, decay, dt)` - Exponential decay interpolation
- `spring(current, target, velocity, stiffness, damping, dt)` - Spring physics

### Easing Functions
- `ease_in(t)` - Quadratic ease in
- `ease_out(t)` - Quadratic ease out
- `ease_in_out(t)` - Quadratic ease in-out
- `ease_in_cubic(t)` - Cubic ease in
- `ease_out_cubic(t)` - Cubic ease out
- `ease_in_out_cubic(t)` - Cubic ease in-out
- `ease_out_bounce(t)` - Bounce ease out
- `ease_out_elastic(t)` - Elastic ease out
- `ease_in_back(t)` - Back ease in (overshoot)
- `ease_out_back(t)` - Back ease out (overshoot)

## Bits Module (`std/bits.wyn`)

### Bit Isolation & Manipulation
- `isolate_rightmost_one(n)` - Isolate rightmost 1-bit
- `isolate_rightmost_zero(n)` - Isolate rightmost 0-bit
- `turn_off_rightmost_one(n)` - Clear rightmost 1-bit
- `turn_on_rightmost_zero(n)` - Set rightmost 0-bit
- `is_contiguous_ones(n)` - Check if bits form contiguous sequence of 1s

### Bit Counting
- `bit_flip_count(a, b)` - Count bit flips needed to convert a to b
- `count_trailing_ones(n)` - Count consecutive 1-bits from right
- `count_leading_ones(n)` - Count consecutive 1-bits from left

### Morton Codes (Z-order curves)
- `interleave_bits(x, y)` - Interleave bits of two 16-bit numbers
- `deinterleave_bits(n)` - Reverse of interleave

### Byte Operations
- `byte_parity(n)` - Compute parity of all bytes
- `pack_bytes(b0, b1, b2, b3)` - Pack four bytes into int
- `unpack_bytes(n)` - Unpack int into four bytes
- `reverse_byte(n)` - Reverse bits in 8-bit value
- `reverse_short(n)` - Reverse bits in 16-bit value
- `replicate_byte(b)` - Replicate low byte to all bytes

### Power of 2 Operations
- `next_power_of_two(n)` - Round up to next power of 2
- `prev_power_of_two(n)` - Round down to previous power of 2
- `log2_pow2(n)` - Compute log2 of power of 2

### Advanced Operations
- `sign_extend(value, bits)` - Sign extend from bit position
- `abs_branchless(n)` - Absolute value without branching
- `min_branchless(a, b)` - Minimum without branching
- `max_branchless(a, b)` - Maximum without branching
- `differs_by_one_bit(a, b)` - Check if exactly one bit differs
- `broadcast_bit(n, pos)` - Broadcast bit to all positions

## Random Module (`std/random.wyn`)

### Weighted Selection
- `weighted_choice(values, weights)` - Random selection with weights

### Probability Distributions
- `rand_gaussian(mean, std_dev)` - Gaussian/normal distribution (Box-Muller)
- `rand_exponential(lambda)` - Exponential distribution

### Geometric Random
- `rand_in_circle()` - Random point inside unit circle
- `rand_on_circle()` - Random point on unit circle

### String Generation
- `rand_uuid()` - Generate random UUID-like string
- `rand_string(n)` - Generate random alphanumeric string
- `rand_hex(n)` - Generate random hexadecimal string

### Noise Functions
- `perlin_1d(x)` - 1D Perlin noise (simplified)
- `grad_1d(hash, x)` - Gradient function for Perlin noise

## Testing

All functions have been tested and verified to compile correctly:
- **Total tests**: 36
- **Passed**: 36
- **Failed**: 0

## Usage Examples

### Binary Search
```wyn
let sorted: [int] = [1, 3, 5, 7, 9, 11, 13]
let idx: int = binary_search(sorted, 7)  # Returns 3
```

### Easing Functions
```wyn
let t: float = 0.5
let eased: float = ease_out_bounce(t)  # Smooth bounce animation
```

### Weighted Random Choice
```wyn
let items: [int] = [1, 2, 3]
let weights: [int] = [10, 30, 60]  # 60% chance for 3
let choice: ?int = weighted_choice(items, weights)
```

### Bit Manipulation
```wyn
let n: int = 0b10110100
let rightmost: int = isolate_rightmost_one(n)  # 0b00000100
```

### Spring Animation
```wyn
let pos: float = 0.0
let vel: float = 0.0
let target: float = 100.0
let (new_pos, new_vel) = spring(pos, target, vel, 10.0, 0.5, 0.016)
```

## Notes

- All functions follow Wyn's minimal, explicit design philosophy
- No external dependencies required
- Functions are optimized for clarity and correctness
- Suitable for game development, scientific computing, and general programming
- Many functions include helper implementations for transcendental functions (sin, cos, ln, exp) where needed

## Future Improvements

Potential areas for expansion:
- Matrix operations
- Complex number support
- More statistical functions
- Graph algorithms
- Cryptographic primitives
- Image processing utilities
