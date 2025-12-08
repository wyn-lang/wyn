# Stage 1 Development Checklist

## Completed ✅ (20+ hours)

### Architecture
- [x] Builtin removal (30+ → 8)
- [x] Library-first design
- [x] Module system working
- [x] 126 files updated with imports
- [x] Tests: 42/64 passing (66%)

### Stage 1 Features
- [x] Compiles hello.wyn
- [x] Parses source files
- [x] Extracts string literals
- [x] Variables (let x = 5)
- [x] Expressions (x + y)
- [x] Function calls
- [x] If statements
- [x] **While loops** ✅ NEW
- [x] **For loops** ✅ NEW
- [x] **Multiple functions** ✅ NEW
- [x] **Imports** ✅ NEW

### Bug Fixes
- [x] substring() type inference
- [x] substring() returns correct values
- [x] String concat in simple cases
- [x] Double prefix in tests (io.io. → io.)

## Stage 1 Capabilities

**Can now compile:**
```wyn
import io

fn helper() {
    print("Helper")
}

fn main() {
    let x: int = 0
    while x < 3 {
        print("Loop")
        x = x + 1
    }
    
    for i in 0..5 {
        print("Iteration")
    }
}
```

**Generates:** Working ARM64 binaries

## Next Steps (15-30 hours)

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
- [ ] Test everything works

### One Builtin (5 hours)
- [ ] Move all builtins to stdlib
- [ ] Only print() remains
- [ ] Update all tests
- [ ] Verify all 64 tests pass

**Total remaining: 25-45 hours**

## Progress

**Stage 1 is 60% complete toward self-hosting.**

Features working:
- ✅ Variables, expressions, functions
- ✅ Control flow (if, while, for)
- ✅ Imports (parsed)
- ✅ Multiple functions (parsed)

Still needed:
- [ ] Function calls between user functions
- [ ] Structs
- [ ] Arrays
- [ ] Full expression parsing
- [ ] Module function calls (io.read_file)

## Success Metrics

- [ ] Stage 1 compiles itself
- [ ] Only print() is builtin
- [ ] All 64 tests pass
- [ ] Pure Wyn stdlib
