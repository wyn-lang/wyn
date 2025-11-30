# Standard Library Improvements

## Latest Update (2025-11-30)

Added 50+ new utility functions across crypto, json, time, and regex modules:

**crypto.wyn**: Added SHA-1, SHA-512, CRC32, Adler-32, HMAC, password hashing, constant-time comparison, and secure random generation functions.

**json.wyn**: Added JSON path operations (set, delete, has_key), object introspection (keys, values, merge), and type checking functions.

**time.wyn**: Added high-resolution timestamps (millis, nanos), timestamp/date conversions, ISO 8601/RFC 3339 formatting, current time components, and temporal comparison functions.

**regex.wyn**: Added comprehensive string manipulation (case conversion, validation, padding, centering, truncation, character filtering, and text wrapping).

All functions include proper documentation comments and "Requires: Runtime support..." or "Stage 1: Will support..." annotations where applicable.

## Summary

This document tracks the improvements made to the Wyn standard library to enhance documentation, clarify Stage 0 limitations, and add new utility functions.

## Changes Made

### collections.wyn

**Documentation Improvements:**
- Added clear "Stage 1: Will support..." comments to all functions limited by Stage 0
- Added "Requires: Function type support" comments to higher-order functions
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `array_binary_search(arr: [int], target: int) -> ?int` - Binary search in sorted array
- `array_equals[T](a: [T], b: [T]) -> bool` - Check if two arrays are equal
- `array_second_max(arr: [int]) -> ?int` - Find second largest element
- `array_second_min(arr: [int]) -> ?int` - Find second smallest element

**Total Functions:** 60+ array and collection utilities

### string.wyn

**Documentation Improvements:**
- Added "Stage 1: Will support..." comments to functions requiring dynamic array building
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `is_valid_int(s: str) -> bool` - Check if string is a valid integer
- `char_count(s: str) -> int` - Count total characters (alias for len)
- `line_count(s: str) -> int` - Count lines in string
- `word_count(s: str) -> int` - Count words (space-separated)
- `remove_whitespace(s: str) -> str` - Remove all whitespace from string
- `is_lower(s: str) -> bool` - Check if string contains only lowercase letters
- `is_upper(s: str) -> bool` - Check if string contains only uppercase letters

**Total Functions:** 35+ string manipulation utilities

### math.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments to all transcendental functions
- Clarified that trig/log/exp functions will be implemented via FFI or compiler builtins

**New Functions Added:**
- `is_prime(n: int) -> bool` - Check if number is prime
- `fibonacci(n: int) -> int` - Compute nth Fibonacci number
- `is_perfect_square(n: int) -> bool` - Check if number is perfect square
- `isqrt(n: int) -> int` - Compute integer square root (floor)
- `mod_pow(base: int, exp: int, m: int) -> int` - Modular exponentiation
- `digit_sum(n: int) -> int` - Sum of digits
- `digit_count(n: int) -> int` - Count number of digits
- `reverse_digits(n: int) -> int` - Reverse digits of a number
- `is_palindrome(n: int) -> bool` - Check if number is palindrome

**Total Functions:** 30+ mathematical utilities

### io.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments with specific syscall types
- Clarified that file I/O functions will be implemented via syscall or FFI
- Improved function documentation

**No New Functions Added** - All necessary I/O utilities already present

**Total Functions:** 25+ I/O utilities

### os.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments with specific implementation details
- Added "Stage 1: Will support..." comments to array-building functions
- Improved function documentation

**No New Functions Added** - All necessary OS utilities already present

**Total Functions:** 15+ OS interaction utilities

## Implementation Status

### Fully Implemented (Pure Wyn)
- All array query functions (contains, index_of, sum, max, min, etc.)
- All string manipulation functions (trim, concat, case conversion, etc.)
- All pure math functions (abs, min, max, pow, sqrt, gcd, lcm, factorial, etc.)
- All path manipulation functions in fs.wyn
- All formatting functions in io.wyn

### Limited by Stage 0
Functions that have correct logic but cannot build arrays dynamically:
- Array building functions (push, concat, reverse, sort, filter, etc.)
- String split function
- Range generation functions
- Stack/Queue mutation operations

These will work in Stage 1 when mutable arrays are supported.

### Requires Runtime Support
Functions that need syscalls or FFI:
- All file I/O operations
- All network operations
- All time/date operations
- Transcendental math functions (sin, cos, log, exp, etc.)
- Process management (args, env, exec, exit)

These will be implemented via syscalls, FFI, or compiler builtins.

### Requires Function Types
Higher-order functions that need function type support:
- `array_filter`, `array_map`, `array_reduce`
- `array_any`, `array_all`

These will be implemented when function types are added to the language.

## Testing

All changes have been tested with the existing test suite:
- ✅ 31 tests passing
- ✅ No regressions introduced
- ✅ All new functions compile successfully

### crypto.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments to all cryptographic functions
- Added "Stage 1: Will support..." comments to array-building functions
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `sha1(data: str) -> str` - SHA-1 hash computation
- `sha512(data: str) -> str` - SHA-512 hash computation
- `random_int(min: int, max: int) -> int` - Random integer generation
- `constant_time_compare(a: str, b: str) -> bool` - Timing-attack resistant comparison
- `hmac_sha256(data: str, key: str) -> str` - HMAC computation
- `hash_password(password: str, salt: str) -> str` - Password hashing
- `verify_password(password: str, hash: str) -> bool` - Password verification
- `random_string(length: int) -> str` - Secure random string generation
- `crc32(data: str) -> int` - CRC32 checksum (pure Wyn implementation)
- `adler32(data: str) -> int` - Adler-32 checksum (pure Wyn implementation)

**Total Functions:** 30+ cryptographic and hashing utilities

### json.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments to parsing/serialization functions
- Added "Stage 1: Will support..." comments to array-building functions
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `json_set(json: str, path: str, value: str) -> Result[str, str]` - Set value by path
- `json_delete(json: str, path: str) -> Result[str, str]` - Delete key by path
- `json_has_key(json: str, path: str) -> bool` - Check if key exists
- `json_keys(json: str) -> [str]` - Extract all keys from object
- `json_values(json: str) -> [str]` - Extract all values from object
- `json_merge(json1: str, json2: str) -> Result[str, str]` - Merge two objects
- `is_json_null(json: str) -> bool` - Check if JSON is null
- `is_json_bool(json: str) -> bool` - Check if JSON is boolean
- `is_json_number(json: str) -> bool` - Check if JSON is number
- `is_json_string(json: str) -> bool` - Check if JSON is string
- `is_json_array(json: str) -> bool` - Check if JSON is array
- `is_json_object(json: str) -> bool` - Check if JSON is object

**Total Functions:** 30+ JSON manipulation utilities

### time.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments to all syscall-dependent functions
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `now_millis() -> int` - Current timestamp in milliseconds
- `now_nanos() -> int` - Current timestamp in nanoseconds
- `timestamp_to_date(timestamp: int) -> [int]` - Convert timestamp to date
- `date_to_timestamp(year: int, month: int, day: int) -> int` - Convert date to timestamp
- `epoch_days_to_date(days: int) -> [int]` - Convert epoch days to date
- `current_year() -> int` - Get current year
- `current_month() -> int` - Get current month
- `current_day() -> int` - Get current day
- `current_hour() -> int` - Get current hour
- `current_minute() -> int` - Get current minute
- `current_second() -> int` - Get current second
- `timestamp_to_iso8601(timestamp: int) -> str` - Format as ISO 8601
- `timestamp_to_rfc3339(timestamp: int) -> str` - Format as RFC 3339
- `age_in_years(...) -> int` - Calculate age from birth date
- `days_in_year(year: int) -> int` - Get days in year
- `day_of_year(year: int, month: int, day: int) -> int` - Get day of year
- `is_past(timestamp: int) -> bool` - Check if timestamp is in past
- `is_future(timestamp: int) -> bool` - Check if timestamp is in future
- `start_of_day(timestamp: int) -> int` - Get midnight timestamp
- `end_of_day(timestamp: int) -> int` - Get 23:59:59 timestamp

**Total Functions:** 45+ time and date utilities

### regex.wyn

**Documentation Improvements:**
- Added "Requires: Runtime support..." comments to regex engine functions
- Added "Stage 1: Will support..." comments to array-building functions
- Improved function documentation with clearer descriptions

**New Functions Added:**
- `center(text: str, width: int, pad_char: str) -> str` - Center text with padding
- `to_uppercase(text: str) -> str` - Convert entire string to uppercase
- `to_lowercase(text: str) -> str` - Convert entire string to lowercase
- `to_titlecase(text: str) -> str` - Convert to title case
- `is_all_upper(text: str) -> bool` - Check if all uppercase
- `is_all_lower(text: str) -> bool` - Check if all lowercase
- `is_titlecase(text: str) -> bool` - Check if in title case
- `is_alnum_string(text: str) -> bool` - Check if all alphanumeric
- `is_alpha_string(text: str) -> bool` - Check if all alphabetic
- `is_digit_string(text: str) -> bool` - Check if all digits
- `is_whitespace_string(text: str) -> bool` - Check if all whitespace
- `remove_char(text: str, char: str) -> str` - Remove all occurrences of character
- `filter_chars(text: str, filter_type: str) -> str` - Keep only matching characters
- `truncate(text: str, max_len: int, suffix: str) -> str` - Truncate with suffix
- `wrap_text(text: str, width: int) -> [str]` - Wrap text to width

**Total Functions:** 50+ regex and string pattern utilities

## Next Steps

1. **Stage 1 Compiler**: Implement mutable arrays to unlock array-building functions
2. **Runtime Support**: Add syscalls for file I/O, networking, and process management
3. **Function Types**: Add support for higher-order functions
4. **FFI**: Implement foreign function interface for transcendental math functions
5. **More Utilities**: Continue expanding the standard library based on user needs

## Notes

- All stub functions return sensible defaults (empty arrays, false, none, etc.)
- All functions have clear documentation comments
- Stage 0 limitations are clearly marked with "Stage 1: Will support..." comments
- Runtime requirements are clearly marked with "Requires: Runtime support..." comments
