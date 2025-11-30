# Standard Library Improvements - 2025-11-30 v6

## Overview
Added 60+ utility functions across string, math, collections, time, and IO modules.

## String Module Enhancements

### Validation Functions
- `is_valid_email()` - Basic email format validation
- `is_valid_url()` - URL protocol validation
- `matches_wildcard()` - Simple wildcard pattern matching (* and ?)

### String Manipulation
- `substring_between()` - Extract text between delimiters
- `swap_case()` - Toggle letter case
- `levenshtein_distance()` - Calculate edit distance between strings

## Math Module Enhancements

### Statistical Functions
- `median()` - Calculate median of sorted array
- `mode()` - Find most frequent value
- `variance()` - Calculate variance
- `std_dev()` - Calculate standard deviation
- `percentile()` - Get percentile value from sorted array

### Geometric Functions
- `deg_to_rad()` / `rad_to_deg()` - Angle conversions
- `hypot()` - Pythagorean hypotenuse
- `distance_2d()` - Euclidean distance between 2D points
- `manhattan_distance()` - Manhattan distance

### Utility Functions
- `normalize_angle()` - Constrain angle to 0-360
- `is_power_of_two()` - Check if number is power of 2
- `round_to_multiple()` - Round to nearest multiple

## Collections Module Enhancements

### Array Analysis
- `array_index_of_max()` / `array_index_of_min()` - Find index of extrema
- `array_is_increasing()` / `array_is_decreasing()` - Check monotonicity
- `array_count_in_range()` - Count elements in range
- `array_nth_largest()` / `array_nth_smallest()` - Find nth element
- `array_count_distinct()` - Count unique elements
- `array_has_duplicates()` - Check for duplicates
- `array_abs_sum()` - Sum of absolute values

## Time Module Enhancements

### Date Navigation
- `start_of_week()` / `end_of_week()` - Week boundaries
- `start_of_month()` / `end_of_month()` - Month boundaries
- `start_of_year()` / `end_of_year()` - Year boundaries
- `add_months()` / `add_years()` - Date arithmetic

### Date Utilities
- `quarter_of_year()` - Get quarter (1-4) from month
- `is_weekend()` / `is_weekday()` - Day type checking
- `business_days_between()` - Count weekdays between dates
- `compare_dates()` - Compare two dates
- `time_ago()` - Human-readable elapsed time
- `format_duration_words()` - Duration in words

## IO Module Enhancements

### Formatting Functions
- `format_bytes()` - Human-readable file sizes (KB, MB, GB)
- `format_elapsed()` - Human-readable time durations
- `format_signed()` - Number with +/- prefix
- `format_yes_no()` - Boolean as Yes/No
- `format_optional_int()` - Optional with N/A fallback
- `format_time()` - HH:MM:SS formatting

### Display Functions
- `print_table_row()` / `print_table_header()` - Table formatting
- `print_progress()` - Progress bar display
- `print_centered()` - Center text in width
- `print_numbered_list()` / `print_bulleted_list()` - List formatting
- `print_banner()` - Title banner with borders

## Testing
All 31 existing tests pass successfully.

## Impact
- String module: +8 functions (validation, parsing, pattern matching)
- Math module: +13 functions (statistics, geometry, utilities)
- Collections module: +8 functions (analysis, searching)
- Time module: +13 functions (navigation, formatting, utilities)
- IO module: +18 functions (formatting, display, tables)

Total: 60 new utility functions added.
