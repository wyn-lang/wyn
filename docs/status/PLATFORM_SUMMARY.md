# Platform Support: Quick Answer

## Question
"Do we need to rewrite for each platform?"

## Short Answer
**No!** Use LLVM backend instead.

## Current vs LLVM

### Current (ARM64 Assembly)
```
Wyn → Parser → ARM64 Assembly → macOS ARM64 only
```
- ✅ Fast prototyping
- ❌ One platform only
- ❌ 9 weeks for 5 platforms

### LLVM Approach
```
Wyn → Parser → LLVM IR → All Platforms
```
- ✅ Write once, run anywhere
- ✅ 1 week for all platforms
- ✅ Free optimizations

## Timeline

| Phase | Coverage | Backend | Duration |
|-------|----------|---------|----------|
| 1 | 0-50% | ARM64 | 2 weeks |
| 2 | 50-70% | + IR layer | 1 week |
| 3 | 70%+ | + LLVM | 1 week |

**Total:** 4 weeks for all platforms

## Example

**Wyn:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

**LLVM IR (works everywhere):**
```llvm
define i64 @add(i64 %a, i64 %b) {
    %result = add i64 %a, %b
    ret i64 %result
}
```

LLVM generates native code for:
- macOS (ARM64, x86)
- Linux (ARM64, x86)
- Windows (x86, ARM64)
- BSD, etc.

## Decision

**Continue ARM64 until 50%**, then add LLVM.

**Result:** Cross-platform support without rewriting!
