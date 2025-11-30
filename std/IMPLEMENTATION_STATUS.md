# Standard Library Implementation Status

## Summary

This document tracks which functions in the standard library are:
- ✅ **Fully implemented** in pure Wyn (Stage 0 features)
- ⚠️ **Limited** by Stage 0 constraints (arrays are immutable, no dynamic building)
- 🔧 **Requires runtime support** (syscalls, FFI, or compiler builtins)

## collections.wyn

### Fully Implemented ✅
- `array_contains` - Check if array contains item
- `array_index_of` - Find first index of item
- `array_first` - Get first element
- `array_last` - Get last element
- `array_take` - Take first n elements (uses slicing)
- `array_skip` - Skip first n elements (uses slicing)
- `array_sum` - Sum all elements
- `array_max` - Find maximum value
- `array_min` - Find minimum value
- `array_avg` - Calculate average
- `array_count` - Count occurrences of value
- `array_is_sorted` - Check if sorted
- `array_slice` - Safe array slicing
- `array_product` - Product of all elements
- `array_is_empty` - Check if empty
- `array_length` - Get length
- `array_last_index_of` - Find last index of item
- `array_starts_with` - Check if starts with prefix
- `array_ends_with` - Check if ends with suffix
- `array_median_sorted` - Get median from sorted array
- `array_count_greater` - Count elements > value
- `array_count_less` - Count elements < value
- `array_range` - Find range (max - min)
- `stack_peek` - Peek at stack top
- `stack_is_empty` - Check if stack empty
- `stack_size` - Get stack size
- `queue_peek` - Peek at queue front
- `queue_is_empty` - Check if queue empty
- `queue_size` - Get queue size

### Limited by Stage 0 ⚠️
These functions have correct logic but cannot build new arrays dynamically:
- `array_push` - Cannot append to arrays
- `array_pop` - Can read last element but cannot remove
- `array_concat` - Cannot build concatenated array
- `array_reverse` - Cannot build reversed array
- `array_sort` - Arrays are immutable
- `array_filter_positive` - Cannot build filtered array
- `array_find_all` - Cannot build result array
- `array_unique` - Cannot build deduplicated array
- `array_chunk_size` - Cannot build chunks
- `array_dedup_sorted` - Cannot build deduplicated array
- `array_rotate_left/right` - Would need concatenation
- `array_swap` - Arrays are immutable
- `array_fill` - Cannot build array
- `array_partition` - Cannot build result arrays
- `range` - Cannot build array
- `range_step` - Cannot build array
- `stack_push/pop` - Cannot modify arrays
- `queue_enqueue/dequeue` - Cannot modify arrays

### Requires Function Types 🔧
- `array_filter` - Needs function type support
- `array_map` - Needs function type support
- `array_reduce` - Needs function type support
- `array_any` - Needs function type support
- `array_all` - Needs function type support

## string.wyn

### Fully Implemented ✅
- `str_len` - Get string length
- `str_concat` - Concatenate strings
- `str_eq` - Compare strings
- `trim` - Trim whitespace from both ends
- `trim_start` - Trim from start
- `trim_end` - Trim from end
- `contains` - Check if contains substring
- `starts_with` - Check if starts with prefix
- `ends_with` - Check if ends with suffix
- `substring` - Extract substring
- `index_of` - Find first occurrence
- `last_index_of` - Find last occurrence
- `repeat` - Repeat string n times
- `reverse` - Reverse string
- `is_empty` - Check if empty
- `char_at` - Get character at index
- `to_upper_str` - Convert to uppercase
- `to_lower_str` - Convert to lowercase
- `to_upper` - Alias for to_upper_str
- `to_lower` - Alias for to_lower_str
- `pad_left` - Pad on left side
- `pad_right` - Pad on right side
- `pad_center` - Center with padding
- `count_substr` - Count substring occurrences
- `is_numeric` - Check if all digits
- `is_alpha` - Check if all alphabetic
- `is_alphanumeric` - Check if alphanumeric
- `is_whitespace` - Check if all whitespace
- `truncate` - Truncate with ellipsis
- `equals_ignore_case` - Case-insensitive comparison
- `join` - Join array of strings

### Limited by Stage 0 ⚠️
- `split` - Cannot build array of strings dynamically
- `replace` - Would need dynamic string building with complex logic

## math.wyn

### Fully Implemented ✅
- `abs` - Absolute value (int)
- `min` - Minimum of two ints
- `max` - Maximum of two ints
- `pow` - Integer power
- `sqrt` - Square root (Newton's method)
- `floor` - Floor function
- `ceil` - Ceiling function
- `round` - Round to nearest int
- `abs_float` - Absolute value (float)
- `min_float` - Minimum of two floats
- `max_float` - Maximum of two floats
- `clamp` - Clamp int to range
- `clamp_float` - Clamp float to range
- `sign` - Sign of number (-1, 0, 1)
- `is_even` - Check if even
- `is_odd` - Check if odd
- `gcd` - Greatest common divisor
- `lcm` - Least common multiple
- `factorial` - Factorial

### Requires Runtime Support 🔧
Transcendental functions need hardware/library support:
- `sin`, `cos`, `tan` - Trigonometric functions
- `asin`, `acos`, `atan`, `atan2` - Inverse trig functions
- `log`, `log10` - Logarithms
- `exp` - Exponential

## io.wyn

### Fully Implemented ✅
- `print_line` - Print with newline
- `get_file_size` - Get file size (uses read_file)
- `format_int` - Format int with zero padding
- `format_bool` - Convert bool to string
- `format_int_padded` - Format int with space padding
- `format_hex` - Format int as hexadecimal
- `format_bin` - Format int as binary
- `print_all` - Print array of strings
- `println_int` - Print int with newline
- `repeat_char` - Repeat character n times

### Requires Runtime Support 🔧
All file I/O operations need syscalls:
- `read_line` - Read from stdin
- `read_file` - Read file contents
- `write_file` - Write to file
- `append_file` - Append to file
- `file_exists` - Check file existence
- `delete_file` - Delete file
- `read_bytes` - Read binary data
- `write_bytes` - Write binary data
- `read_lines` - Read file as lines
- `create_dir` - Create directory
- `remove_dir` - Remove directory
- `list_dir` - List directory contents
- `copy_file` - Copy file
- `move_file` - Move file

## os.wyn

### Fully Implemented ✅
- `get_arg` - Get command-line arg by index (uses args())
- `arg_count` - Get number of args (uses args())
- `has_env` - Check if env var exists (uses getenv())
- `getenv_or` - Get env var with default (uses getenv())
- `has_arg` - Check if arg exists
- `program_name` - Get program name
- `get_arg_int` - Parse int from arg
- `has_flag` - Check for flag
- `get_flag_value` - Get value after flag
- `get_args_from` - Get args from index (limited by array building)
- `get_args` - Get all args except program name (limited)
- `has_any_flag` - Check for any flag from list
- `getenv_int` - Parse int from env var
- `is_debug` - Check DEBUG env var

### Requires Runtime Support 🔧
- `args` - Get command-line arguments
- `env` - Get all environment variables
- `getenv` - Get environment variable
- `setenv` - Set environment variable
- `exit` - Exit program
- `exec` - Execute external command

## fs.wyn

### Fully Implemented ✅
All path manipulation functions are pure Wyn:
- `join_path` - Join two path components
- `dirname` - Get directory portion
- `basename` - Get filename portion
- `extension` - Get file extension
- `normalize_path` - Normalize path
- `remove_extension` - Remove extension
- `is_absolute` - Check if absolute path
- `has_extension` - Check if has extension
- `join_paths` - Join multiple paths
- `parent` - Get parent directory
- `stem` - Get filename without extension
- `split_path` - Split into dir and filename
- `has_trailing_slash` - Check for trailing slash
- `ensure_trailing_slash` - Add trailing slash
- `remove_trailing_slash` - Remove trailing slash
- `has_ext` - Check if matches extension
- `with_extension` - Replace extension
- `path_depth` - Count path components
- `is_filename_only` - Check if just filename

### Requires Runtime Support 🔧
All file system operations need syscalls:
- `read` - Read file
- `write` - Write file
- `append` - Append to file
- `exists` - Check existence
- `mkdir` - Create directory
- `rmdir` - Remove directory
- `remove` - Remove file
- `list` - List directory
- `is_file` - Check if file
- `is_dir` - Check if directory
- `stat` - Get file metadata

## Other Libraries

### net.wyn 🔧
All networking functions require runtime support (sockets, syscalls)

### http.wyn 🔧
All HTTP functions require runtime support (networking + parsing)

### time.wyn 🔧
All time functions require runtime support (system clock, sleep)

### json.wyn 🔧
JSON parsing/serialization requires complex parsing (could be implemented but complex)

### regex.wyn 🔧
Regex requires complex parsing and matching engine

### crypto.wyn 🔧
Cryptographic functions require specialized algorithms and hardware support

### test.wyn 🔧
Testing framework requires runtime support for test discovery and execution

### log.wyn 🔧
Logging requires I/O operations

## Stage 0 Limitations

The main limitation preventing more implementations is:

**Arrays are immutable and cannot be dynamically built**

Stage 0 supports:
- ✅ Array literals: `[1, 2, 3]`
- ✅ Array indexing: `arr[i]`
- ✅ Array slicing: `arr[1:3]`
- ✅ Array length: `arr.len()`
- ❌ Array append/push
- ❌ Array element assignment: `arr[i] = value`
- ❌ Dynamic array building in loops

Once Stage 1 adds mutable arrays or array builders, many more functions can be implemented.
