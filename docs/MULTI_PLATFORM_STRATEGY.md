# Multi-Platform Strategy

## Current Problem

Stage 2 generates hardcoded ARM64 assembly:
```wyn
emit(out, "    mov x19, #5")  # ARM64 specific!
```

**Issue:** Need to rewrite for x86, Windows, Linux, etc.

## Solution: Intermediate Representation (IR)

### Architecture

```
Wyn Source → Parser → IR → Backend → Assembly
                              ↓
                         ARM64/x86/LLVM
```

### IR Example

Instead of:
```wyn
emit(out, "    mov x19, #5")
```

Use:
```wyn
ir_assign("x", 5)
ir_call("print", ["x"])
```

Then backends translate:
- **ARM64:** `mov x19, #5`
- **x86:** `mov rax, 5`
- **LLVM:** `%x = alloca i64` + `store i64 5, i64* %x`

## Implementation Plan

### Phase 1: IR Layer (Now)
```wyn
# IR operations
fn ir_const(name: str, val: int)
fn ir_binop(op: str, a: str, b: str) -> str
fn ir_call(func: str, args: [str])
fn ir_if(cond: str, then_block: [IR], else_block: [IR])
```

### Phase 2: Backends
```wyn
# Backend interface
trait Backend {
    fn emit_const(name: str, val: int)
    fn emit_binop(op: str, a: str, b: str)
    fn emit_call(func: str, args: [str])
}

# Implementations
struct ARM64Backend implements Backend { ... }
struct X86Backend implements Backend { ... }
struct LLVMBackend implements Backend { ... }
```

### Phase 3: Target Selection
```bash
./stage2 --target=arm64 input.wyn
./stage2 --target=x86 input.wyn
./stage2 --target=llvm input.wyn
```

## Minimal IR Design

### Core Operations (10)
1. `const` - Constants
2. `assign` - Variables
3. `binop` - Binary operations
4. `call` - Function calls
5. `ret` - Returns
6. `label` - Jump targets
7. `jump` - Unconditional jump
8. `branch` - Conditional jump
9. `load` - Memory load
10. `store` - Memory store

### Example

**Wyn:**
```wyn
let x: int = 5
let y: int = x + 3
print(y)
```

**IR:**
```
const x, 5
binop y, x, +, 3
call print, [y]
```

**ARM64:**
```asm
mov x19, #5
mov x20, x19
add x20, x20, #3
mov x0, x20
bl _puts
```

**x86:**
```asm
mov rax, 5
mov rbx, rax
add rbx, 3
mov rdi, rbx
call puts
```

## Better Approach: Use LLVM

### Why LLVM?

1. **One backend** - LLVM handles all platforms
2. **Optimizations** - Free optimizations
3. **Mature** - Battle-tested
4. **Cross-platform** - Works everywhere

### LLVM IR Example

**Wyn:**
```wyn
fn add(a: int, b: int) -> int {
    return a + b
}
```

**LLVM IR:**
```llvm
define i64 @add(i64 %a, i64 %b) {
    %result = add i64 %a, %b
    ret i64 %result
}
```

### Implementation

```wyn
fn emit_llvm(out: str) {
    io.append_file(out, "define i64 @add(i64 %a, i64 %b) {\n")
    io.append_file(out, "    %result = add i64 %a, %b\n")
    io.append_file(out, "    ret i64 %result\n")
    io.append_file(out, "}\n")
}

fn compile_llvm(src: str) {
    emit_llvm("output.ll")
    os.system("llc -o output.s output.ll")  # LLVM → assembly
    os.system("cc -o output output.s")       # assembly → binary
}
```

## Recommended Path

### Short Term (Now - 50%)
Continue with ARM64 to prove features work.

### Medium Term (50% - 70%)
Add IR layer:
```wyn
# stage2_ir.wyn
fn compile(src: str) {
    const ir: [IR] = parse_to_ir(src)
    const asm: str = ir_to_arm64(ir)
    write_file("output.s", asm)
}
```

### Long Term (70% - 100%)
Switch to LLVM:
```wyn
# stage2_llvm.wyn
fn compile(src: str) {
    const ir: [IR] = parse_to_ir(src)
    const llvm: str = ir_to_llvm(ir)
    write_file("output.ll", llvm)
    os.system("llc output.ll")
}
```

## Comparison

| Approach | Platforms | Effort | Optimizations |
|----------|-----------|--------|---------------|
| Direct ASM | 1 | Low | Manual |
| IR + Backends | Many | High | Manual |
| LLVM | All | Medium | Free |

## Decision

**Recommendation:** Use LLVM

**Reasons:**
1. One backend for all platforms
2. Free optimizations
3. Industry standard
4. Less code to maintain

**Timeline:**
- Now - 50%: ARM64 (prove features)
- 50% - 70%: Add IR layer
- 70%+: Switch to LLVM

## Example: Stage 2 with LLVM

```wyn
# stage2_llvm.wyn
import io
import os

fn emit_llvm_function(out: str, name: str) {
    io.append_file(out, "define i64 @")
    io.append_file(out, name)
    io.append_file(out, "() {\n")
    io.append_file(out, "    ret i64 0\n")
    io.append_file(out, "}\n")
}

fn compile(src: str) {
    io.write_file("output.ll", "")
    emit_llvm_function("output.ll", "main")
    
    # LLVM → native
    os.system("llc -o output.s output.ll")
    os.system("cc -o output output.s")
}

fn main() {
    compile("input.wyn")
}
```

## Conclusion

**Current:** Hardcoded ARM64 (fast to prototype)
**Future:** LLVM backend (cross-platform)

**Action:** Continue ARM64 until 50%, then add LLVM backend.

This avoids rewriting for each platform!
