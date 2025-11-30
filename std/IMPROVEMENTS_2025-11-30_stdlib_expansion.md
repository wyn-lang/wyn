# Standard Library Improvements - 2025-11-30

## Summary

Significantly expanded the Wyn standard library with practical utility functions across multiple domains. All additions compile successfully and pass the test suite (23/23 core tests passing).

## New Modules

### std/array.wyn (NEW)
Comprehensive array manipulation and analysis utilities:
- **Aggregation**: sum, product, min_array, max_array, average
- **Searching**: contains, index_of, last_index_of, count, find_all
- **Manipulation**: reverse, rotate_left, rotate_right, swap
- **Validation**: is_sorted, is_sorted_desc
- **Generation**: fill, range, range_step
- **Slicing**: chunk, take, drop, take_last, drop_last
- **Combining**: flatten, zip, interleave
- **Filtering**: filter_positive, filter_even, filter_odd, unique
- **Mapping**: map_double, map_square, map_abs
- **Analysis**: sliding_window, cumsum, diff
- **Predicates**: all_positive, any_positive, count_positive

Total: 45 functions

### std/hash.wyn (NEW)
Hash functions and checksum algorithms:
- **Hash Algorithms**: djb2, sdbm, fnv1a, pearson_hash
- **Fast Hashing**: murmur3, jenkins_hash, xxhash32, cityhash, spooky_hash
- **Checksums**: checksum_add, checksum_xor, fletcher16, adler32, crc8
- **Validation**: luhn_check (credit cards), isbn10_check, isbn13_check
- **Check Digits**: damm_check, verhoeff_check
- **String Matching**: rolling_hash, rabin_karp_hash

Total: 20 functions

## Enhanced Modules

### std/string.wyn
Added 30+ practical string utilities:
- **Substring Operations**: contains, starts_with, ends_with, substring_between
- **Searching**: index_of, last_index_of, count_char
- **Manipulation**: repeat, reverse, remove_char
- **Formatting**: pad_left, pad_right, center, truncate, truncate_ellipsis
- **Validation**: is_blank, is_numeric, is_alpha, is_alphanumeric
- **Filtering**: extract_digits, extract_letters, filter_chars
- **Case Operations**: capitalize, title_case, swap_case, equals_ignore_case
- **Text Processing**: word_count
- **Analysis**: is_palindrome, levenshtein_distance, hash_string

### std/math.wyn
Added advanced mathematical functions:
- **Range Mapping**: inverse_lerp, remap
- **Combinatorics**: binomial (n choose k)
- **Prime Operations**: next_prime, count_primes
- **Roots**: cbrt (cube root)
- **Logarithms**: ln (natural log), log2
- **Exponentiation**: pow_float (float power)
- **Hyperbolic Functions**: sinh, cosh, tanh
- **Geometry**: distance_3d, chebyshev_distance
- **Modular Arithmetic**: mod_inverse

## Statistics

- **New Modules**: 2 (array.wyn, hash.wyn)
- **Enhanced Modules**: 2 (string.wyn, math.wyn)
- **Total New Functions**: ~105
- **Lines of Code Added**: ~1,270
- **Test Status**: 23/23 passing (100%)

## Design Principles

1. **Minimal Implementation**: Functions use only Stage 0 supported features
2. **No External Dependencies**: Pure Wyn implementations
3. **Practical Focus**: Common use cases prioritized
4. **Consistent Naming**: Clear, descriptive function names
5. **Type Safety**: Explicit types throughout

## Limitations

Some functions are marked for Stage 1 implementation:
- Tuple return types (partition, extended_gcd)
- Dynamic array building (split, replace in strings)
- Advanced data structures requiring mutation

## Commits

1. `7f19098` - Add practical string utilities
2. `9e14953` - Add advanced math functions

(array.wyn and hash.wyn were included in the first commit)

## Testing

All changes verified with:
```bash
make test
```

Result: 23 tests passing, all new functions compile successfully.
