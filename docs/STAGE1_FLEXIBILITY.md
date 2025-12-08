# Stage 1 Compiler Flexibility Update

## Problem
The Stage 1 compiler had hardcoded message indices (msg0, msg1, msg2) causing linker errors when the actual number of print statements didn't match expectations.

## Solution
Made all three compilation functions dynamically handle varying numbers of print statements:

### Changes Made

1. **compile_simple_program()** - Now supports 1-8 print statements
2. **compile_while_program()** - Now supports 1-5 print statements in loops
3. **compile_for_program()** - Now supports 1-5 print statements in loops

### Implementation
- Used `count_strings()` to determine actual message count
- Replaced hardcoded message indices with conditional generation based on count
- Used if statements instead of while loops (Stage 0 has issues with nested while loops)

### Tested Scenarios
✅ Simple programs: 1, 3, 5, 8 print statements
✅ While loops: 1, 2, 3, 4 print statements
✅ For loops: 1, 2, 3 print statements

### Limitations
- Simple programs: max 8 prints
- Loop programs: max 5 prints per iteration
- Loop bounds still hardcoded (while x < 3, for i in 0..3)

### Future Work
- Parse actual loop bounds from source
- Support unlimited print statements (requires proper while loop support in Stage 0)
- Add support for other statement types beyond print
