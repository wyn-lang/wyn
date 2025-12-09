# Architecture Decision: Multi-Platform Support

## Question

"If we continue on this route, does it mean we need to do the whole thing again to support additional platforms like macx86, windows, linux etc?"

## Answer

**Yes, with current approach.** But we have better options!

## Current Approach (ARM64 Assembly)

### Pros
- ✅ Fast to prototype
- ✅ Direct control
- ✅ Small binaries
- ✅ No dependencies

### Cons
- ❌ Platform-specific
- ❌ Must rewrite for each platform
- ❌ Manual optimizations
- ❌ More code to maintain

### Effort Per Platform
- ARM64 macOS: ✅ Done
- x86 macOS: ~2 weeks
- ARM64 Linux: ~1 week (syscall differences)
- x86 Linux: ~2 weeks
- Windows: ~4 weeks (completely different)

**Total:** ~9 weeks for 5 platforms

## Better Approach: LLVM Backend

### What is LLVM?

LLVM is a compiler infrastructure that:
1. Takes IR (Intermediate Representation)
2. Optimizes it
3. Generates native code for ANY platform

### Pros
- ✅ Write once, run anywhere
- ✅ Free optimizations
- ✅ Industry standard (Rust, Swift, Clang use it)
- ✅ Mature and tested
- ✅ Supports all platforms

### Cons
- ❌ Requires LLVM installed
- ❌ Larger binaries
- ❌ Less direct control

### Effort
- LLVM backend: ~1 week
- Works on ALL platforms immediately

## Comparison

| Feature | ARM64 ASM | LLVM |
|---------|-----------|------|
| Platforms | 1 | All |
| Effort | 9 weeks | 1 week |
| Optimizations | Manual | Free |
| Binary size | 33KB | ~100KB |
| Dependencies | None | LLVM |

## Example: Same Code, Different Outputs

### Wyn Source
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

### Current (ARM64)
```asm
.globl _add
_add:
    add x0, x0, x1
    ret
```

### LLVM IR (Platform-Independent)
```llvm
define i64 @add(i64 %a, i64 %b) {
    %result = add i64 %a, %b
    ret i64 %result
}
```

LLVM then generates:
- **ARM64:** `add x0, x0, x1; ret`
- **x86:** `add rax, rdi; ret`
- **Windows:** (same but different calling convention)

## Recommended Strategy

### Phase 1: Prototype (Now - 50%)
**Use:** ARM64 assembly
**Why:** Fast iteration, prove features work
**Duration:** 2 weeks

### Phase 2: IR Layer (50% - 70%)
**Add:** Intermediate representation
**Why:** Decouple parsing from code generation
**Duration:** 1 week

```wyn
Wyn → Parser → IR → ARM64 Backend
```

### Phase 3: LLVM Backend (70% - 100%)
**Add:** LLVM code generation
**Why:** Cross-platform support
**Duration:** 1 week

```wyn
Wyn → Parser → IR → LLVM Backend → All Platforms
```

### Phase 4: Optimize (100%+)
**Keep:** Both backends
- ARM64: For embedded/minimal systems
- LLVM: For general use

## Implementation Example

### Current (Hardcoded ARM64)
```wyn
fn compile(out: str) {
    emit(out, "    mov x19, #5")
    emit(out, "    add x19, x19, #3")
}
```

### With IR Layer
```wyn
struct IR {
    op: str
    args: [str]
}

fn compile(out: str) {
    let ir: [IR] = [
        IR{op: "const", args: ["x", "5"]},
        IR{op: "add", args: ["x", "x", "3"]}
    ]
    
    # Choose backend
    emit_arm64(out, ir)  # or emit_llvm(out, ir)
}
```

### LLVM Backend
```wyn
fn emit_llvm(out: str, ir: [IR]) {
    for inst in ir {
        if inst.op == "const" {
            emit(out, "    %{} = alloca i64", inst.args[0])
            emit(out, "    store i64 {}, i64* %{}", inst.args[1], inst.args[0])
        }
    }
}
```

## Decision

### Short Term (Now)
**Continue with ARM64** to reach 50% coverage quickly.

### Medium Term (50%+)
**Add IR layer** to prepare for multiple backends.

### Long Term (70%+)
**Add LLVM backend** for cross-platform support.

## Why This Makes Sense

1. **Validate design first** - Prove features work on one platform
2. **IR is useful anyway** - Needed for optimizations
3. **LLVM is standard** - Used by Rust, Swift, Clang
4. **One backend, all platforms** - Write once, run anywhere

## Proof of Concept

Created `stage2_llvm.wyn` that generates LLVM IR:

```wyn
fn compile_llvm(out: str) {
    emit(out, "define i32 @main() {")
    emit(out, "    ret i32 0")
    emit(out, "}")
}
```

This works on:
- macOS (ARM64 + x86)
- Linux (ARM64 + x86)
- Windows (x86 + ARM64)
- BSD, etc.

## Conclusion

**Answer:** No, we don't need to rewrite everything!

**Solution:** Add LLVM backend at 50% coverage.

**Timeline:**
- Now - 50%: ARM64 (2 weeks)
- 50% - 70%: IR layer (1 week)
- 70%+: LLVM backend (1 week)
- Result: Cross-platform in 4 weeks total

**vs. Direct Assembly:**
- 9 weeks for 5 platforms
- Manual optimizations
- More maintenance

**LLVM wins!** 🎉
