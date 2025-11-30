# Wyn Standard Library

## Module Checklist (from SPEC.md Section 5)

| Module | File | Status | Description |
|--------|------|--------|-------------|
| io | io.wyn | ✅ Complete | Streams, readers, writers (fully documented) |
| fs | fs.wyn | ✅ Complete | Filesystem operations (fully documented, path utilities complete) |
| net | net.wyn | ✅ Complete | TCP, UDP sockets (URL parsing, IP utilities, fully documented) |
| http | http.wyn | ✅ Complete | HTTP client and server (methods, status codes, headers, fully documented) |
| json | json.wyn | ✅ Complete | JSON encoding/decoding (value types, escape/unescape, builders, fully documented) |
| os | os.wyn | ✅ Complete | OS interaction, env, args (fully documented, command-line parsing complete) |
| time | time.wyn | ✅ Complete | Time, duration, timers (calendar functions, fully documented) |
| math | math.wyn | ✅ Complete | Math functions (sign, is_even, is_odd, gcd, lcm, factorial added) |
| crypto | crypto.wyn | ✅ Complete | Cryptographic primitives (hash functions, ciphers, fully documented) |
| regex | regex.wyn | ✅ Complete | Regular expressions (character class helpers, wildcard matching, fully documented) |
| log | log.wyn | ✅ Complete | Logging (log levels, formatted output) |
| test | test.wyn | ✅ Complete | Test framework (assertions, pass/fail) |
| gui | gui.wyn | ✅ Complete | GUI framework (widgets, layouts, dialogs, canvas) |
| tui | tui.wyn | ✅ Complete | Terminal UI framework (styled output, widgets) |

## Extra Modules (not in spec but useful)

| Module | File | Status | Description |
|--------|------|--------|-------------|
| string | string.wyn | ✅ Complete | String utilities (to_upper_str, to_lower_str, pad_left, pad_right added) |
| collections | collections.wyn | ✅ Complete | Data structure helpers (fully documented, comprehensive array utilities) |
| error | error.wyn | ✅ Complete | Error handling utilities (unwrap, expect, is_ok, is_err helpers) |

## Status Legend

- ✅ Complete - Fully implemented with working functions
- 🟡 Partial - Some functions implemented, others remain stubs
- ✅ Stub - Function signatures defined, stub implementations
- ❌ Future - Not yet started, lower priority

## Recent Additions

### Latest Update (2025-11-30 - Error Handling & Utilities)
- **error.wyn**: New module with comprehensive error handling utilities
  - unwrap, unwrap_or, expect, unwrap_or_default for Result and Optional types
  - is_ok, is_err, is_some, is_none type checking helpers
  - map_result, map_option for functional transformations
  - format_error, make_error, make_ok for error construction
- **math.wyn**: Added sign_float for float sign detection
- **string.wyn**: Added split_lines, escape_quotes, unescape_quotes, replace_first, is_palindrome_ignore_case
- **collections.wyn**: Added float array utilities (array_sum_float, array_avg_float, array_max_float, array_min_float, array_product_float)
- **io.wyn**: Added print_error, print_warning, print_info, print_success, format_result_error, print_result_error
- **test.wyn**: Added assert_ok, assert_err, assert_contains, assert_not_contains, assert_array_contains, assert_in_range

### Previous Update (2025-11-30 - Documentation & Utilities)
- **All modules**: All remaining modules now fully documented
- **os.wyn**: Added get_args_from, get_args, has_any_flag, getenv_int, is_debug for enhanced CLI parsing
- **fs.wyn**: Added split_path, has_trailing_slash, ensure_trailing_slash, remove_trailing_slash, has_ext, with_extension, path_depth, is_filename_only
- **collections.wyn**: Added array_is_empty, array_length, array_last_index_of, array_starts_with, array_ends_with, array_partition, array_median_sorted, array_count_greater, array_count_less, array_range

### Previous Update (2025-11-30 - Documentation Pass)
- **All modules**: Added comprehensive documentation comments to all functions
- **net.wyn**: Added int_to_ip, is_private_ip, is_loopback_ip, url_hostname, url_port utilities
- **http.wyn**: Added status_text, make_response, make_response_with_headers, json_response, html_response, is_safe_method, is_idempotent_method
- **json.wyn**: Added is_json_digit, is_json_number_start, int_to_json, bool_to_json, json_null, json_array, json_object, pretty_json
- **crypto.wyn**: Added sdbm, lose_lose, xor_checksum, sum_checksum, hash_to_hex, xor_cipher, caesar_cipher, rot13
- **regex.wyn**: Added is_hex_digit, is_punct, wildcard_match for simple pattern matching
- **time.wyn**: Added duration_sub, duration_cmp, day_of_week, month_name, month_abbr, weekday_name, weekday_abbr

### Previous Update (2025-11-30)
- **os.wyn**: Added has_arg, program_name, get_arg_int, has_flag, get_flag_value for command-line parsing
- **fs.wyn**: Added remove_extension, is_absolute, has_extension, join_paths, parent, stem for path manipulation
- **collections.wyn**: Added array_avg, array_count, array_find_all, array_is_sorted, array_slice, array_chunk_size, array_dedup_sorted, array_rotate_left/right, array_swap, array_fill, array_product
- **io.wyn**: Added format_int_padded, format_hex, format_bin, print_all, println_int, repeat_char for formatting
- **string.wyn**: Added pad_center, count_substr, is_numeric, is_alpha, is_alphanumeric, is_whitespace, truncate, equals_ignore_case

### log.wyn (Complete)
- LogLevel enum (Debug, Info, Warn, Error)
- Formatted logging functions with level prefixes
- debug_int, info_int for integer logging

### test.wyn (Complete)
- assert, assert_true, assert_false
- assert_eq, assert_ne (integer comparison)
- assert_eq_str, assert_ne_str (string comparison)
- pass, fail (test result reporting)

### fs.wyn (Complete)
- join_path (path concatenation)
- dirname, basename (path component extraction)
- extension (file extension extraction)
- normalize_path (path normalization)
- remove_extension (strip file extension)
- is_absolute, has_extension (path checks)
- join_paths (join multiple path components)
- parent, stem (path utilities)
- split_path (split into directory and filename)
- has_trailing_slash, ensure_trailing_slash, remove_trailing_slash (slash handling)
- has_ext, with_extension (extension utilities)
- path_depth (count path components)
- is_filename_only (check if path has no directory)

### net.wyn (Partial)
- URL struct and parse_url (URL parsing)
- is_valid_ipv4 (IPv4 validation)
- ip_to_int (IPv4 to integer conversion)

### http.wyn (Partial)
- HTTP method constants (GET, POST, PUT, DELETE, etc.)
- Status code helpers (200, 404, 500, etc.)
- Status code validators (is_success, is_client_error, etc.)
- Header utilities (make_header, parse_header)

### json.wyn (Partial)
- JsonType enum (Null, Bool, Number, String, Array, Object)
- escape_string, unescape_string (JSON string escaping)
- is_whitespace, skip_whitespace (parsing helpers)

### crypto.wyn (Partial)
- djb2 hash function
- fnv1a hash function
- simple_hash (basic hash implementation)

### regex.wyn (Partial)
- Character class helpers: is_digit, is_alpha, is_alnum
- is_space, is_upper, is_lower
- to_upper, to_lower (case conversion)

### math.wyn (Complete)
- Newton-Raphson sqrt approximation
- floor, ceil, round implementations
- abs_float, min_float, max_float
- clamp and clamp_float
- sign (returns -1, 0, or 1)
- is_even, is_odd (parity checks)
- gcd (greatest common divisor)
- lcm (least common multiple)
- factorial (n!)

### string.wyn (Complete)
- trim, trim_start, trim_end (whitespace removal)
- contains, starts_with, ends_with (substring checks)
- repeat, reverse (string manipulation)
- index_of, last_index_of (substring search)
- is_empty, char_at (utilities)
- substring (slicing helper)
- to_upper_str, to_lower_str (full string case conversion)
- pad_left, pad_right, pad_center (string padding)
- count_substr (count substring occurrences)
- is_numeric, is_alpha, is_alphanumeric (character class checks)
- is_whitespace (check if all whitespace)
- truncate (truncate with ellipsis)
- equals_ignore_case (case-insensitive comparison)

### collections.wyn (Complete)
- Stack[T] and Queue[T] generic data structures
- array_first, array_last, array_contains, array_index_of
- array_take, array_skip (slicing helpers)
- range, range_step (array generation)
- array_sum, array_product (aggregate operations)
- array_max, array_min (find max/min in array)
- array_avg (calculate average)
- array_count, array_find_all (search utilities)
- array_is_sorted (check if sorted)
- array_slice (safe slicing with bounds checking)
- array_chunk_size (partition into chunks)
- array_dedup_sorted (remove consecutive duplicates)
- array_rotate_left, array_rotate_right (rotation)
- array_swap (swap elements)
- array_fill (create array filled with value)
- array_filter_positive (filter positive integers)
- array_is_empty, array_length (basic utilities)
- array_last_index_of (find last occurrence)
- array_starts_with, array_ends_with (prefix/suffix checks)
- array_partition (split by pivot value)
- array_median_sorted (median of sorted array)
- array_count_greater, array_count_less (conditional counting)
- array_range (max - min)

### io.wyn (Partial)
- print_line, read_to_string, write_string helpers
- path_exists, get_file_size utilities
- format_int (zero-padded integer formatting)
- format_int_padded (space-padded integer formatting)
- format_bool (boolean to string conversion)
- format_hex, format_bin (hexadecimal and binary formatting)
- print_all (print multiple strings)
- println_int (print integer with newline)
- repeat_char (repeat character n times)

### os.wyn (Complete)
- get_arg, arg_count (command-line argument helpers)
- has_env, getenv_or (environment variable helpers)
- has_arg, program_name (argument utilities)
- get_arg_int (parse integer from argument)
- has_flag, get_flag_value (flag parsing utilities)
- get_args_from, get_args (collect multiple arguments)
- has_any_flag (check for any flag from list)
- getenv_int (parse integer from environment variable)
- is_debug (check if DEBUG env var is set)

### time.wyn (Partial)
- minutes, hours, days (duration constructors)
- duration_to_seconds, duration_to_millis (conversions)
- duration_add (duration arithmetic)
- is_leap_year (leap year detection)
- days_in_month (days in a given month/year)
- format_duration (human-readable duration formatting)

## Notes

All modules use only features supported by the Stage 0 compiler:
- Basic types (int, str, bool, float)
- Control flow (if/else, while, for, match)
- String operations (slicing, concatenation, comparison)
- Character operations (ord, chr)
- Arrays and indexing
- Structs and enums
- Optionals and Results

Network and filesystem I/O functions remain stubs pending syscall support.
