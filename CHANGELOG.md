# Changelog

## [0.5.0] - 2024-12-31

### Changed
- Renamed 'let' keyword to 'var' for variable declarations

### Added
- List comprehensions for functional programming style
- Slice syntax for arrays and strings
- String methods:
  - `.find()` - locate substring position
  - `.substring()` - extract substring
  - `.char_at()` - get character at index
- Array methods:
  - `.contains()` - check if element exists
  - `.prepend()` - add element to beginning
- VSCode extension for syntax highlighting and language support

### Fixed
- String and array iteration now works correctly
- `.join()` method for arrays
- `.contains()` method reliability

### Testing
- 90/90 tests passing (100% test coverage)