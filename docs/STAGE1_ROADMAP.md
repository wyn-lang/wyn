# Stage 1 Compiler - Development Roadmap

## Goal

Build a self-hosting Wyn compiler written in Wyn.

## Current State

- **stage0:** Working C bootstrap compiler
- **Tests:** 41/64 passing (64%)
- **Builtins:** 6 (minimal)
- **Architecture:** Library-first, ready for Stage 1

## Stage 1 Plan

### Phase 1: Minimal Compiler (20-30 hours)

Build a Wyn compiler in Wyn that can compile hello.wyn.

**Components:**
1. **Lexer** (4 hours) - Tokenize Wyn source
2. **Parser** (6 hours) - Build AST
3. **Type Checker** (4 hours) - Validate types
4. **Codegen** (8 hours) - Generate ARM64 assembly
5. **Integration** (3 hours) - Wire it together
6. **Testing** (5 hours) - Debug and fix

**Deliverable:**
```bash
./build/stage1 -o hello hello.wyn
./hello
# Output: Hello, World!
```

### Phase 2: Self-Hosting (10-15 hours)

Stage 1 compiles itself.

**Steps:**
1. Ensure Stage 1 supports all features it uses
2. Compile Stage 1 source with Stage 1
3. Verify output matches
4. Bootstrap complete

**Deliverable:**
```bash
# Compile Stage 1 with stage0
./build/stage0 -o stage1_v1 src/stage1/compiler.wyn

# Compile Stage 1 with itself
./stage1_v1 -o stage1_v2 src/stage1/compiler.wyn

# They're equivalent
diff stage1_v1 stage1_v2
```

### Phase 3: Pure Wyn (10-15 hours)

Remove C runtime, use syscalls.

**Steps:**
1. Add `syscall()` primitive to Stage 1
2. Rewrite `std/io.wyn` with syscalls
3. Rewrite `std/os.wyn` with syscalls
4. Rewrite `std/time.wyn` with syscalls
5. Remove `runtime/builtins.c`

**Deliverable:**
```wyn
import io  // Pure Wyn, no C

fn main() {
    const data = io.read_file("file.txt")  // Direct syscall
    print(data)
}
```

## Timeline

- Phase 1: 20-30 hours (3-4 days)
- Phase 2: 10-15 hours (1-2 days)
- Phase 3: 10-15 hours (1-2 days)

**Total: 40-60 hours (1-2 weeks)**

## Workarounds for stage0 Bugs

stage0 has bugs with:
- String returns from functions
- Array indexing beyond index 1

**Workarounds:**
- Use global variables instead of function returns
- Hardcode values instead of using args()
- Keep Stage 1 simple initially

## Success Criteria

✅ Stage 1 compiles hello.wyn
✅ Stage 1 compiles itself
✅ Stage 1 output matches stage0 output
✅ Pure Wyn stdlib with syscalls

## After Stage 1

- Add optimizations
- Add more backends (x86_64, WASM, LLVM)
- Better error messages
- IDE support (LSP)
- Package manager

But first: **Get Stage 1 self-hosting.**
