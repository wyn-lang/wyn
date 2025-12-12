# math Module

std/math.wyn
Mathematical functions and utilities
Return absolute value of an integer
Return minimum of two integers
Return maximum of two integers

## Functions

### `fn abs(n: int) -> int`

### `fn min(a: int, b: int) -> int`

### `fn max(a: int, b: int) -> int`

### `fn pow(base: int, exp: int) -> int`

### `fn sqrt(n: float) -> float`

### `fn floor(n: float) -> int`

### `fn ceil(n: float) -> int`

### `fn round(n: float) -> int`

### `fn sin(x: float) -> float`

### `fn cos(x: float) -> float`

### `fn tan(x: float) -> float`

### `fn asin(x: float) -> float`

### `fn acos(x: float) -> float`

### `fn atan(x: float) -> float`

### `fn atan2(y: float, x: float) -> float`

### `fn log(x: float) -> float`

### `fn log10(x: float) -> float`

### `fn exp(x: float) -> float`

### `fn abs_float(n: float) -> float`

### `fn min_float(a: float, b: float) -> float`

### `fn max_float(a: float, b: float) -> float`

### `fn clamp(value: int, min_val: int, max_val: int) -> int`

### `fn clamp_float(value: float, min_val: float, max_val: float) -> float`

### `fn sign(n: int) -> int`

### `fn sign_float(n: float) -> float`

### `fn is_even(n: int) -> bool`

### `fn is_odd(n: int) -> bool`

### `fn gcd(a: int, b: int) -> int`

### `fn lcm(a: int, b: int) -> int`

### `fn factorial(n: int) -> int`

### `fn is_prime(n: int) -> bool`

### `fn fibonacci(n: int) -> int`

### `fn is_perfect_square(n: int) -> bool`

### `fn isqrt(n: int) -> int`

### `fn mod_pow(base: int, exp: int, m: int) -> int`

### `fn digit_sum(n: int) -> int`

### `fn digit_count(n: int) -> int`

### `fn reverse_digits(n: int) -> int`

### `fn is_palindrome(n: int) -> bool`

### `fn lerp(a: float, b: float, t: float) -> float`

### `fn map_range(value: float, in_min: float, in_max: float, out_min: float, out_max: float) -> float`

### `fn avg(a: int, b: int) -> int`

### `fn avg_float(a: float, b: float) -> float`

### `fn sum_range(start: int, end: int) -> int`

### `fn product_range(start: int, end: int) -> int`

### `fn median(arr: [int]) -> ?int`

### `fn mode(arr: [int]) -> ?int`

### `fn variance(arr: [int]) -> ?float`

### `fn std_dev(arr: [int]) -> ?float`

### `fn percentile(arr: [int], p: int) -> ?int`

### `fn normalize_angle(degrees: int) -> int`

### `fn deg_to_rad(degrees: float) -> float`

### `fn is_pythagorean_triple(a: int, b: int, c: int) -> bool`

### `fn collatz_length(n: int) -> int`

### `fn divisor_sum(n: int) -> int`

### `fn is_abundant(n: int) -> bool`

### `fn is_deficient(n: int) -> bool`

### `fn hypot(a: float, b: float) -> float`

### `fn distance_2d(x1: float, y1: float, x2: float, y2: float) -> float`

### `fn manhattan_distance(x1: int, y1: int, x2: int, y2: int) -> int`

### `fn is_power_of_two(n: int) -> bool`

### `fn round_to_multiple(value: int, n: int) -> int`

### `fn divmod(a: int, b: int) -> (int, int)`

### `fn next_power_of_two(n: int) -> int`

### `fn trailing_zeros(n: int) -> int`

### `fn leading_zeros(n: int) -> int`

### `fn popcount(n: int) -> int`

### `fn round_nearest(x: float) -> int`

### `fn floor_float(x: float) -> int`

### `fn ceil_float(x: float) -> int`

### `fn trunc(x: float) -> int`

### `fn fract(x: float) -> float`

### `fn array_average(arr: [int]) -> float`

### `fn array_median(sorted_arr: [int]) -> float`

### `fn array_std_dev(arr: [int]) -> float`

### `fn array_variance(arr: [int]) -> float`

### `fn inverse_lerp(a: float, b: float, value: float) -> float`

### `fn remap(value: float, in_min: float, in_max: float, out_min: float, out_max: float) -> float`

### `fn binomial(n: int, k: int) -> int`

### `fn next_prime(n: int) -> int`

### `fn count_primes(n: int) -> int`

### `fn cbrt(n: float) -> float`

### `fn ln(x: float) -> float`

### `fn log2(x: float) -> float`

### `fn pow_float(base: float, exp: float) -> float`

### `fn sinh(x: float) -> float`

### `fn cosh(x: float) -> float`

### `fn tanh(x: float) -> float`

### `fn distance_3d(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> float`

### `fn chebyshev_distance(x1: int, y1: int, x2: int, y2: int) -> int`

### `fn mod_inverse(a: int, m: int) -> int`

### `fn is_perfect(n: int) -> bool`

### `fn triangular(n: int) -> int`

### `fn pentagonal(n: int) -> int`

### `fn hexagonal(n: int) -> int`

### `fn is_triangular(n: int) -> bool`

### `fn catalan(n: int) -> int`

### `fn bell(n: int) -> int`

### `fn stirling_second(n: int, k: int) -> int`

### `fn harmonic(n: int) -> float`

### `fn totient(n: int) -> int`

### `fn mobius(n: int) -> int`

### `fn partition_count(n: int) -> int`

### `fn fill(size: int, value: int) -> [int]`

### `fn smoothstep(edge0: float, edge1: float, x: float) -> float`

### `fn smootherstep(edge0: float, edge1: float, x: float) -> float`

### `fn sigmoid(x: float) -> float`

### `fn relu(x: float) -> float`

### `fn leaky_relu(x: float, alpha: float) -> float`

### `fn softplus(x: float) -> float`

### `fn gaussian(x: float, mu: float, sigma: float) -> float`

