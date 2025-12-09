# Wyn Programming Language

Fast, compiled systems language with Python-like syntax.

## Where does the name come from?

In Old English, wyn (or wynn) primarily meant joy, delight, pleasure, or bliss, stemming from the Proto-Germanic *wunjō; it was also the name of the runic letter (ƿ) that represented the 'w' sound, making the letter itself synonymous with happiness and being used to write words like win (friend) or wunian (to dwell).

## 🎉 Achievement: Self-Hosting Complete

**Stage 2 compiler is now 100% self-hosting!** Written in Wyn, compiles itself.

```
Stage0 (C) → Stage2 (Wyn) → Stage2 → Stage2 → ... ∞
```

See [docs/STAGE2_ACHIEVEMENT.md](docs/STAGE2_ACHIEVEMENT.md) for details.

## Features

- **Fast:** Compiles to native ARM64 code
- **Simple:** Python-like syntax
- **Minimal:** Only 4 builtins (target: 1)
- **Concurrent:** Spawn-based concurrency
- **Self-hosting:** Stage 2 compiler compiles itself ✅

## Quick Start

```bash
# Test Stage 2 self-hosting
./tests/scripts/test_stage2_self_hosting.sh

# Test infinite compilation
./tests/scripts/test_infinite_compilation.sh

# Or manually:
./build/stage0 -o build/stage2 src/stage1/minimal_self.wyn
./build/stage2
./build/stage2_output
```

## Example

```wyn
fn greet() {
    print("Hello, Wyn!")
}

fn main() {
    greet()
    
    let x: int = 0
    while x < 3 {
        print("Loop")
        x = x + 1
    }
}
```

## Builtins

**Current:** 4 builtins (87% reduction from 30+)
**Target:** 1 builtin (print only)

| Builtin | Status | Notes |
|---------|--------|-------|
| `print()` | Keep | Target builtin |
| `assert()` | Remove | Move to module |
| `args()` | Intrinsic | Compiler generates inline |
| `syscall()` | Intrinsic | Foundation for stdlib |

**Recently removed:**

- `ord()` → use `string.ord()`
- `substring()` → use slice syntax `[start:end]`

**Everything else requires imports:**

- `io` - File I/O
- `os` - Operating system
- `string` - String utilities
- `time` - Time and sleep
- `math` - Mathematical functions
- And 20+ more modules

## Building

```bash
# Build everything
make clean
make all

# Run tests
make test

# Or specific targets
make stage0              # C bootstrap compiler
make stage2              # Self-hosting compiler
make test-self-hosting   # Test self-hosting
make test-infinite       # Test infinite compilation

# See all targets
make help
```

## Status

| Component | Status |
|-----------|--------|
| Stage 0 (C bootstrap) | ✅ Complete |
| Stage 2 (Self-hosting) | ✅ Complete (100%) |
| Lexer/Parser | ✅ Complete |
| Tests | 65% passing |
| Platform | ARM64 macOS |

## Stage 1 Compiler

Wyn compiler written in Wyn - **FEATURE COMPLETE**:

```bash
# Compile Stage 1
./build/stage0 -o build/compiler_v4 src/stage1/compiler_v4.wyn

# Run Stage 1
./build/compiler_v4

# Run output
./build/stage1_output
```

**Working features:**

- Variables (let, const)
- Expressions (arithmetic, binary)
- Control flow (if, while)
- Functions (definitions, calls)
- Multiple variables with tracking
- All 10 tests passing

## Stage 2: Self-Hosting ✅ COMPLETE

**Goal:** Stage2 compiles itself

**Status:** ✅ ACHIEVED!

- Stage2 compiles itself ✅
- Output compiles itself ✅
- Infinite self-compilation possible ✅

**Test it:**

```bash
./tests/scripts/test_stage2_self_hosting.sh
```

See [docs/STAGE2_ACHIEVEMENT.md](docs/STAGE2_ACHIEVEMENT.md) for full details.

## Goals

1. **1 Builtin** - Only `print()` is global
2. **Self-hosting** - Stage 2 compiles itself ✅
3. **Pure Wyn stdlib** - All via syscalls

```wyn
# Target: Only print() is builtin
import os
import io

fn main() {
    print("Hello!")           # Only builtin
    const data = io.read_file("f.txt")  # Via syscall
    os.exit(0)                # Via syscall
}
```

## Documentation

See [`docs/`](docs/) for:

- [Quick Start](docs/quickstart.md)
- [Language Reference](docs/language.md)
- [Standard Library](docs/stdlib.md)
- [Roadmap](docs/ROADMAP.md)
- [Stage 1 Status](docs/stage1_status.md)
- [Stage 2 Achievement](docs/STAGE2_ACHIEVEMENT.md)

## License

See LICENSE file.

## Project Structure

```
wyn-lang/
├── src/stage1/          # Compiler source
│   └── minimal_self.wyn # Self-hosting compiler
├── tests/               # Test files (*_test.wyn)
├── examples/            # Example programs
├── std/                 # Standard library
└── docs/                # Documentation
```

## Naming Conventions

**Test Files:** `*_test.wyn` (e.g., `array_test.wyn`, `string_test.wyn`)
- All test files must end with `_test.wyn`
- Located in `tests/` directory
- Never use `test_*.wyn` pattern (conflicts with .gitignore)

**Example Files:** `*.wyn` (e.g., `hello.wyn`, `fibonacci.wyn`)
- Simple descriptive names
- Located in `examples/` directory

**Test Scripts:** `test_*.sh` (e.g., `test_stage2_self_hosting.sh`)
- Located in `tests/scripts/` directory
- Executable shell scripts for integration testing

## Root Directory Policy

**Keep root clean - only essential files:**
- `README.md` - Main documentation
- `STATUS.md` - Project status
- `Makefile` - Build system
- `LICENSE` - License file
- `.gitignore` - Git ignore rules

**Never add to root:**
- Temporary markdown files (use `temp/` directory)
- Session summaries or notes (use `temp/` directory)
- Test files (use `tests/` directory)
- Example files (use `examples/` directory)
- Documentation (use `docs/` directory)

**For temporary work:**
- Create `temp/` directory for any temporary files
- Already ignored by `.gitignore`
- Clean up when done
