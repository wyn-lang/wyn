# Wyn Standard Library - Quick Reference

**Total Functions: 749** across 17 modules

## Core Modules (from SPEC.md)

### io.wyn - Input/Output Operations
- File I/O: `read_to_string`, `write_string`, `read_file`, `write_file`, `append_file`
- Console: `print_line`, `print_error`, `print_warning`, `print_info`, `print_success`
- Formatting: `format_int`, `format_hex`, `format_bin`, `format_thousands`
- Utilities: `print_box`, `print_line_char`, `repeat_char`

### fs.wyn - Filesystem Operations
- Path manipulation: `join_path`, `dirname`, `basename`, `extension`, `normalize_path`
- Path utilities: `split_path`, `parent`, `stem`, `is_absolute`, `has_extension`
- File operations: `delete_file`, `copy_file`, `move_file`, `path_exists`
- Directory: `create_dir`, `remove_dir`, `list_dir`

### net.wyn - Network Operations
- URL parsing: `parse_url`, `url_hostname`, `url_port`
- IP utilities: `is_valid_ipv4`, `ip_to_int`, `int_to_ip`, `is_private_ip`, `is_loopback_ip`
- Sockets: TCP, UDP (stub implementations)

### http.wyn - HTTP Client/Server
- Methods: `GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `HEAD`, `OPTIONS`
- Status codes: `status_ok`, `status_not_found`, `status_error`, `status_text`
- Responses: `make_response`, `json_response`, `html_response`
- Validators: `is_success`, `is_client_error`, `is_server_error`

### json.wyn - JSON Encoding/Decoding
- Types: `JsonType` enum (Null, Bool, Number, String, Array, Object)
- Escaping: `escape_string`, `unescape_string`
- Builders: `int_to_json`, `bool_to_json`, `json_null`, `json_array`, `json_object`
- Formatting: `pretty_json`

### os.wyn - Operating System Interaction
- Environment: `getenv`, `getenv_int`, `setenv`, `is_debug`
- Arguments: `get_args`, `get_arg`, `get_arg_int`, `has_arg`, `program_name`
- Flags: `has_flag`, `get_flag_value`, `has_any_flag`
- Process: `exit_code`, `current_dir`, `home_dir`

### time.wyn - Time and Duration
- Time: `now`, `unix_timestamp`, `format_time`, `parse_time`
- Duration: `duration_seconds`, `duration_minutes`, `duration_hours`, `duration_days`
- Calendar: `day_of_week`, `month_name`, `weekday_name`, `is_leap_year`
- Utilities: `sleep`, `measure_time`

### math.wyn - Mathematical Functions
- Basic: `abs`, `min`, `max`, `pow`, `sqrt`, `sign`, `sign_float`
- Rounding: `floor`, `ceil`, `round`
- Trigonometry: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
- Number theory: `gcd`, `lcm`, `factorial`, `is_prime`, `fibonacci`
- Utilities: `clamp`, `lerp`, `map_range`, `avg`, `is_even`, `is_odd`

### crypto.wyn - Cryptographic Primitives
- Hashing: `djb2`, `fnv1a`, `sdbm`, `lose_lose`, `hash_to_hex`
- Checksums: `xor_checksum`, `sum_checksum`
- Ciphers: `xor_cipher`, `caesar_cipher`, `rot13`
- Secure: SHA-256, AES (stub implementations)

### regex.wyn - Regular Expressions
- Character classes: `is_digit`, `is_alpha`, `is_alnum`, `is_hex_digit`, `is_punct`
- Matching: `wildcard_match` (simple pattern matching)
- Full regex: (stub implementations for Stage 1+)

### log.wyn - Logging Framework
- Levels: `LogLevel` enum (Debug, Info, Warn, Error)
- Functions: `debug`, `info`, `warn`, `error`
- Formatted: `debug_int`, `info_int`, `warn_int`, `error_int`

### test.wyn - Testing Framework
- Basic: `assert`, `assert_true`, `assert_false`, `pass`, `fail`
- Comparison: `assert_eq`, `assert_ne`, `assert_gt`, `assert_lt`, `assert_ge`, `assert_le`
- Strings: `assert_eq_str`, `assert_ne_str`, `assert_contains`, `assert_not_contains`
- Types: `assert_some`, `assert_none`, `assert_ok`, `assert_err`
- Arrays: `assert_len_eq`, `assert_array_contains`, `assert_in_range`

### gui.wyn - GUI Framework
- Widgets: `Button`, `Label`, `TextBox`, `CheckBox`, `RadioButton`, `Slider`
- Layouts: `HBox`, `VBox`, `Grid`
- Dialogs: `show_message`, `show_error`, `show_confirm`, `show_input`
- Canvas: Drawing primitives

### tui.wyn - Terminal UI Framework
- Styling: `style_bold`, `style_italic`, `style_underline`
- Colors: `color_red`, `color_green`, `color_blue`, `color_yellow`
- Widgets: `draw_box`, `draw_progress`, `draw_table`
- Input: `read_key`, `clear_screen`

### mobile.wyn - Mobile Development
- Platform: `is_ios`, `is_android`, `platform_name`
- Sensors: `get_location`, `get_accelerometer`, `get_gyroscope`
- UI: `show_toast`, `vibrate`, `open_url`
- Storage: `save_preference`, `load_preference`

## Extra Modules

### string.wyn - String Utilities
- Case: `to_upper`, `to_lower`, `capitalize`, `title_case`
- Trimming: `trim`, `trim_start`, `trim_end`
- Searching: `contains`, `starts_with`, `ends_with`, `index_of`, `last_index_of`
- Manipulation: `repeat`, `reverse`, `truncate`, `replace_first`
- Padding: `pad_left`, `pad_right`, `pad_center`
- Validation: `is_numeric`, `is_alpha`, `is_alphanumeric`, `is_whitespace`
- Escaping: `escape_quotes`, `unescape_quotes`
- Counting: `char_count`, `line_count`, `word_count`, `count_substr`

### collections.wyn - Data Structures
- Stack: `stack_new`, `stack_push`, `stack_pop`, `stack_peek`, `stack_is_empty`
- Queue: `queue_new`, `queue_enqueue`, `queue_dequeue`, `queue_peek`, `queue_is_empty`
- Array utilities: `array_first`, `array_last`, `array_contains`, `array_index_of`
- Array math: `array_sum`, `array_avg`, `array_max`, `array_min`, `array_product`
- Float arrays: `array_sum_float`, `array_avg_float`, `array_max_float`, `array_min_float`
- Array operations: `array_reverse`, `array_sort`, `array_unique`, `array_concat`
- Array slicing: `array_take`, `array_skip`, `array_slice`, `array_chunk_size`
- Array search: `array_find_all`, `array_binary_search`, `array_is_sorted`

### error.wyn - Error Handling
- Unwrapping: `unwrap`, `unwrap_or`, `expect`, `unwrap_or_default`
- Type checking: `is_ok`, `is_err`, `is_some`, `is_none`
- Mapping: `map_result`, `map_option`
- Construction: `format_error`, `make_error`, `make_ok`

## Common Patterns

### Error Handling
```wyn
import error

# Unwrap or exit
let content: str = unwrap(read_file("config.txt"), "Failed to read config")

# Unwrap or use default
let port: int = unwrap_or(parse_int(env), 8080)

# Check before using
if is_ok(result) {
    let value: int = result!
}
```

### String Processing
```wyn
import string

let text: str = "  Hello World  "
let clean: str = trim(text)
let upper: str = to_upper(clean)
let padded: str = pad_left(upper, 20, " ")
```

### Array Operations
```wyn
import collections

let nums: [int] = [1, 2, 3, 4, 5]
let sum: int = array_sum(nums)
let avg: ?int = array_avg(nums)
let max: ?int = array_max(nums)
```

### Testing
```wyn
import test

fn test_addition() {
    assert_eq(add(2, 3), 5, "2 + 3 should equal 5")
    assert_gt(add(5, 5), 9, "5 + 5 should be greater than 9")
    pass("Addition test passed")
}
```

### File I/O
```wyn
import io
import fs

let result: Result[str, str] = read_to_string("data.txt")
match result {
    ok(content) => print_success("File loaded")
    err(e) => print_error("Failed: " + e)
}
```

## Module Import Guide

```wyn
# Import single module
import math

# Use functions from imported module
let result: int = abs(-42)

# Import multiple modules
import string
import collections
import error

# Use functions
let text: str = trim("  hello  ")
let nums: [int] = [1, 2, 3]
let sum: int = array_sum(nums)
```

## Stage 0 Limitations

Some functions are stubs awaiting Stage 1 features:
- Dynamic array building: `split`, `array_filter`, `array_map`, `array_concat`
- Function types: Higher-order functions like `map`, `filter`, `reduce`
- Advanced I/O: Async operations, streaming
- Network: Full socket implementation
- Regex: Full pattern matching engine

These will be implemented as the compiler gains more capabilities.
