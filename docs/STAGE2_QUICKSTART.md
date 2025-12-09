# Stage2 Quick Start Guide

## Current Status

✅ **Stage1:** Compiler written in Wyn, compiles simple programs
❌ **Stage2:** Compiler cannot yet compile itself

## The Problem

compiler_v4 uses features it doesn't implement:
- String slicing: `src[pos:pos+1]`
- String methods: `.len()`
- Complex expressions

## The Solution

Create **compiler_v5** using only features compiler_v4 implements:

### Allowed Features
✅ Variables: `let x: int = 0`, `const name: str = "test"`
✅ If statements: `if x == 0 { ... }`
✅ While loops: `while x < 10 { ... }`
✅ Functions: `fn name(param: type) -> type { ... }`
✅ Print: `print("text")`
✅ Builtins: `substring(str, start, end)`, `ord(char)`
✅ Imports: `import io`, `import os`, `import string`
✅ Simple expressions: `x + 1`, `a == b`

### Forbidden Features (for now)
❌ String slicing: `str[i:j]` → use `substring(str, i, j)`
❌ String methods: `.len()` → use `len(str)` builtin
❌ Complex expressions: `a + b + c` → break into steps
❌ Nested calls: `f(g(x))` → use temp variables

## Example Transformation

### Before (compiler_v4 - doesn't work)
```wyn
fn parse(src: str) -> str {
    let pos: int = 0
    while pos < src.len() {
        if src[pos:pos+1] == "l" {
            return src[pos:pos+3]
        }
        pos = pos + 1
    }
    return ""
}
```

### After (compiler_v5 - works)
```wyn
fn parse(src: str) -> str {
    let pos: int = 0
    const src_len: int = len(src)
    
    while pos < src_len {
        const end: int = pos + 1
        const c: str = substring(src, pos, end)
        
        if c == "l" {
            const end3: int = pos + 3
            const result: str = substring(src, pos, end3)
            return result
        }
        
        pos = pos + 1
    }
    
    return ""
}
```

## Implementation Checklist

### Phase 1: Create compiler_v5.wyn
- [ ] Copy compiler_v4.wyn to compiler_v5.wyn
- [ ] Replace all `str[i:j]` with `substring(str, i, j)`
- [ ] Replace all `.len()` with `len(str)`
- [ ] Break complex expressions into simple steps
- [ ] One operation per line
- [ ] Add temp variables as needed

### Phase 2: Test
- [ ] Compile with Stage0: `./build/stage0 -o build/compiler_v5 src/stage1/compiler_v5.wyn`
- [ ] Test with simple program: `./build/compiler_v5`
- [ ] Verify output works

### Phase 3: Self-Compile
- [ ] Modify compiler_v5 to read its own source
- [ ] Run: `./build/compiler_v5` (compiles itself)
- [ ] Compile output: `cc -o build/stage2 build/stage2_output.s`
- [ ] Test: `./build/stage2` (should work!)

### Phase 4: Validate
- [ ] Compare Stage0 vs Stage1 output
- [ ] Test with all examples
- [ ] Document differences
- [ ] Fix bugs

## Success Criteria

✅ compiler_v5 compiles its own source
✅ Output binary works correctly
✅ Can compile other programs
✅ Foundation for adding features

## Next Steps After Self-Hosting

Once Stage2 works, incrementally add features:
1. String slicing
2. Method calls
3. Complex expressions
4. Arrays
5. For loops
6. Structs
7. ... eventually 100% language coverage

## Time Estimate

- Phase 1: 2-3 hours
- Phase 2: 30 minutes
- Phase 3: 30 minutes
- Phase 4: 1 hour
- **Total: 4-5 hours**

## Key Principle

**Keep it simple!** The goal is self-hosting, not features. Add features later, one at a time, while maintaining self-hosting at each step.
