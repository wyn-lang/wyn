# Stage 1 Development Checklist

## Completed ✅ (18 hours)

### Architecture
- [x] Builtin removal (30+ → 8)
- [x] Library-first design
- [x] Module system working
- [x] 126 files updated with imports

### Stage 1 Basics
- [x] Project structure created
- [x] Compiles with stage0
- [x] Generates ARM64 assembly
- [x] Creates working binaries
- [x] Parses source files
- [x] Extracts string literals
- [x] Handles multiple print statements

### Bug Fixes
- [x] substring() type inference
- [x] substring() returns correct values

## In Progress ⏳

### Stage 1 Features (25-40 hours remaining)

#### Parser (10-15 hours)
- [ ] Parse variables: `let x: int = 5`
- [ ] Parse expressions: `x + y`, `x * 2`
- [ ] Parse function calls: `add(x, y)`
- [ ] Parse if statements: `if x > 0 { ... }`
- [ ] Parse while loops: `while x < 10 { ... }`
- [ ] Parse for loops: `for i in 0..10 { ... }`
- [ ] Parse structs: `struct Point { x: int }`
- [ ] Parse imports: `import io`
- [ ] Build proper AST

#### Code Generator (15-20 hours)
- [ ] Generate variable declarations
- [ ] Generate variable assignments
- [ ] Generate arithmetic expressions
- [ ] Generate comparisons
- [ ] Generate if/else branches
- [ ] Generate while loops
- [ ] Generate for loops
- [ ] Generate function calls
- [ ] Generate struct access
- [ ] Generate module calls

#### Testing (5-10 hours)
- [ ] Test with variables
- [ ] Test with expressions
- [ ] Test with functions
- [ ] Test with control flow
- [ ] Test with imports
- [ ] Fix bugs

### Self-Hosting (10-15 hours)
- [ ] Ensure Stage 1 supports all features it uses
- [ ] Compile Stage 1 source with Stage 1
- [ ] Debug and fix issues
- [ ] Verify output matches
- [ ] Bootstrap complete

### Syscalls (10-15 hours)
- [ ] Add syscall() primitive to Stage 1
- [ ] Rewrite std/os.wyn with syscalls
- [ ] Rewrite std/io.wyn with syscalls
- [ ] Rewrite std/time.wyn with syscalls
- [ ] Test everything works

### One Builtin (5-10 hours)
- [ ] Move assert() to std/test.wyn
- [ ] Move exit() to std/os.wyn
- [ ] Move args() to std/os.wyn
- [ ] Move int_to_str() to std/string.wyn
- [ ] Move system() to std/os.wyn
- [ ] Move write_file() to std/io.wyn
- [ ] Move substring() to string methods
- [ ] Move ord() to std/string.wyn
- [ ] Update Stage 1 to use imports
- [ ] Reduce builtins to print() only
- [ ] Update all tests
- [ ] Verify all 64 tests pass

## Known Issues

### stage0 Bugs (Not Blocking)
- [x] substring() - FIXED
- [ ] String concat in functions - Workaround: use io.append_file()
- [ ] Array of strings - Workaround: process inline
- [ ] Module constants - Workaround: use literals

### To Fix in Stage 1
- [ ] Proper string handling
- [ ] Proper array handling
- [ ] Full type inference
- [ ] Better error messages

## Time Estimates

- Parser expansion: 10-15 hours
- Codegen expansion: 15-20 hours
- Testing: 5-10 hours
- Self-hosting: 10-15 hours
- Syscalls: 10-15 hours
- One builtin: 5-10 hours

**Total remaining: 55-85 hours (1-2 weeks)**

## Next Immediate Task

**Add variable support to Stage 1 (3-5 hours)**

1. Parse `let x: int = 5`
2. Generate code to store variable
3. Parse `print(x)`
4. Generate code to load and print variable
5. Test

Then: expressions, function calls, control flow, etc.

## Success Criteria

- [ ] Stage 1 compiles itself
- [ ] Only print() is builtin
- [ ] All 64 tests pass
- [ ] Pure Wyn stdlib
- [ ] Native performance

## Current File

Working on: `src/stage1/compiler.wyn` (currently 100 lines)
