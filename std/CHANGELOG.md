# Wyn Standard Library Changelog

## 2025-11-30 - Documentation & Utilities Expansion

### Documentation
- Added comprehensive documentation comments to all functions in `os.wyn`
- Added comprehensive documentation comments to all functions in `fs.wyn`
- Added comprehensive documentation comments to all functions in `collections.wyn`
- All three modules now have clear function descriptions and parameter documentation

### os.wyn - New Functions
- `get_args_from(start: int) -> [str]` - Get all arguments after index
- `get_args() -> [str]` - Get all arguments except program name
- `has_any_flag(flags: [str]) -> bool` - Check if any flag from list is present
- `getenv_int(key: str) -> ?int` - Get environment variable as integer
- `is_debug() -> bool` - Check if running in debug mode (DEBUG env var set)

### fs.wyn - New Functions
- `split_path(path: str) -> (str, str)` - Split path into directory and filename
- `has_trailing_slash(path: str) -> bool` - Check if path ends with separator
- `ensure_trailing_slash(path: str) -> str` - Add trailing slash if not present
- `remove_trailing_slash(path: str) -> str` - Remove trailing slash if present
- `has_ext(path: str, ext: str) -> bool` - Check if path matches extension
- `with_extension(path: str, new_ext: str) -> str` - Replace extension with new one
- `path_depth(path: str) -> int` - Count path components (depth)
- `is_filename_only(path: str) -> bool` - Check if path is just a filename

### collections.wyn - New Functions
- `array_is_empty[T](arr: [T]) -> bool` - Check if array is empty
- `array_length[T](arr: [T]) -> int` - Get array length (alias for .len())
- `array_last_index_of(arr: [int], value: int) -> ?int` - Find last index of item
- `array_starts_with(arr: [int], prefix: [int]) -> bool` - Check if array starts with prefix
- `array_ends_with(arr: [int], suffix: [int]) -> bool` - Check if array ends with suffix
- `array_partition(arr: [int], pivot: int) -> ([int], [int])` - Partition array by pivot value
- `array_median_sorted(arr: [int]) -> ?int` - Get median value from sorted array
- `array_count_greater(arr: [int], value: int) -> int` - Count elements greater than value
- `array_count_less(arr: [int], value: int) -> int` - Count elements less than value
- `array_range(arr: [int]) -> ?int` - Find range (max - min) of array

### Status Updates
- `os.wyn`: 🟡 Partial → ✅ Complete
- `fs.wyn`: 🟡 Partial → ✅ Complete
- `collections.wyn`: 🟡 Partial → ✅ Complete

### Implementation Notes
All new functions use only Stage 0 compiler features:
- Basic types (int, str, bool)
- Control flow (if/else, while, for, match)
- String operations (slicing, concatenation, comparison)
- Character operations (ord, chr)
- Arrays and indexing
- Structs and optionals
- No external dependencies

### Total Functions Added
- os.wyn: 5 new utility functions
- fs.wyn: 8 new path manipulation functions
- collections.wyn: 10 new array utility functions
- **Total: 23 new functions**

### Documentation Coverage
- All 15 standard library modules now have complete documentation
- Every function has a clear description
- Parameter and return value documentation included
- Usage examples in comments where helpful
