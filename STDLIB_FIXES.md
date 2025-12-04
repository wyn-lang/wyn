# Standard Library Implementation Fixes

## Summary

Fixed unimplemented placeholder functions across the Wyn standard library that were returning dummy values (0, "", etc.).

## Files Modified

### 1. std/time.wyn
**Fixed Functions:**
- `now()` - Now returns `time_now()` builtin instead of 0
- `now_millis()` - Calculates milliseconds from `time_now()`
- `now_nanos()` - Calculates nanoseconds from `time_now()`
- `sleep(duration)` - Now calls `sleep_ms()` builtin with converted duration
- `current_year()` - Calculates from `now()` using `timestamp_to_date()`
- `current_month()` - Calculates from `now()` using `timestamp_to_date()`
- `current_day()` - Calculates from `now()` using `timestamp_to_date()`
- `current_hour()` - Calculates from `now()` timestamp
- `current_minute()` - Calculates from `now()` timestamp
- `current_second()` - Calculates from `now()` timestamp
- `format(timestamp, fmt)` - Returns ISO 8601 formatted string
- `parse(time_str, fmt)` - Parses YYYY-MM-DD format dates

### 2. std/gpu.wyn
**Fixed Functions:**
- `current_device()` - Now calls `gpu_current_device()` external function
- `set_device(device_id)` - Now calls `gpu_set_device()` external function
- `memory_free()` - Now calls `gpu_memory_free()` external function
- `memory_total()` - Now calls `gpu_memory_total()` external function
- `Tensor.sum()` - Now calls `gpu_tensor_sum()` external function
- `Tensor.mean()` - Now calls `gpu_tensor_mean()` external function
- `Tensor.max()` - Now calls `gpu_tensor_max()` external function
- `Tensor.min()` - Now calls `gpu_tensor_min()` external function

### 3. std/mobile.wyn
**Fixed Functions:**
- `os_version()` - Now calls `mobile_os_version()` external function
- `documents_dir()` - Now calls `mobile_documents_dir()` external function
- `cache_dir()` - Now calls `mobile_cache_dir()` external function
- `temp_dir()` - Now calls `mobile_temp_dir()` external function

### 4. std/io.wyn
**Fixed Functions:**
- `read_line()` - Now calls `read_stdin_line()` external function

## Already Implemented (No Changes Needed)

The following functions were found to be properly implemented:
- **std/math.wyn**: `lcm()`, `factorial()`, `fibonacci()`, `isqrt()`, `mod_pow()`, `sum_range()` - all working
- **std/string.wyn**: `line_count()`, `word_count()` - properly implemented
- **std/regex.wyn**: `index_of()`, `count_occurrences()` - properly implemented
- **std/collections.wyn**: `array_chunk_count()`, `array_product()` - properly implemented
- **std/fs.wyn**: `path_depth()` - properly implemented
- **std/encoding.wyn**: `base64_value()`, `hex_value()` - return 0 as fallback for invalid input (correct behavior)

## Implementation Notes

1. **Builtin Functions Used:**
   - `time_now()` - Returns current Unix timestamp in seconds
   - `sleep_ms(ms)` - Sleeps for specified milliseconds
   - `read_stdin_line()` - Reads a line from stdin

2. **External Functions Expected:**
   - GPU functions: `gpu_current_device()`, `gpu_set_device()`, `gpu_memory_free()`, `gpu_memory_total()`, `gpu_tensor_sum()`, `gpu_tensor_mean()`, `gpu_tensor_max()`, `gpu_tensor_min()`
   - Mobile functions: `mobile_os_version()`, `mobile_documents_dir()`, `mobile_cache_dir()`, `mobile_temp_dir()`

3. **Functions Still Limited:**
   - `time.format()` - Only supports ISO 8601 output (not custom format strings)
   - `time.parse()` - Only supports YYYY-MM-DD format
   - These could be enhanced with more sophisticated string manipulation when available

## Testing

The compiler builds successfully with all changes:
```bash
make clean && make all
```

Build output shows no errors, only expected warnings in C code.
