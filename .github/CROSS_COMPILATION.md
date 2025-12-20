# Cross-Compilation Strategy

## Problem

GitHub Actions has limited availability for certain runners:
- `macos-15-latest` (Intel Macs) - Queue times: 20+ hours
- `ubuntu-22.04-arm64` (ARM64 Linux) - Queue times: 20+ hours

This causes release builds to stall for nearly a day.

## Solution: Cross-Compilation

Use LLVM's native cross-compilation capabilities to build all architectures from readily available runners.

### Native Builds (Fast Runners)

These platforms build natively on their target architecture:

| Platform | Runner | Build Time | Availability |
|----------|--------|------------|--------------|
| macOS ARM64 | macos-14 | ~5 min | ✅ Excellent |
| Linux x86_64 | ubuntu-22.04 | ~3 min | ✅ Excellent |
| Windows x86_64 | windows-2022 | ~5 min | ✅ Excellent |

### Cross-Compiled Builds (From Fast Runners)

These platforms are cross-compiled from available runners:

| Target Platform | Host Runner | Cross-Compile Command | Build Time |
|----------------|-------------|----------------------|------------|
| macOS x86_64 | macos-14 (ARM64) | `clang -target x86_64-apple-macos11.0` | ~3 min |
| Linux ARM64 | ubuntu-22.04 (x86_64) | `clang --target=aarch64-linux-gnu` | ~3 min |

## How Cross-Compilation Works

### macOS x86_64 (from ARM64)

```bash
# On macOS ARM64 runner
clang -target x86_64-apple-macos11.0 \
  -Wall -Wextra -std=c11 -O2 -DCOMPILER_MAIN \
  -o build/wyn src/wyn.c src/compiler.c

# Produces x86_64 binary
file build/wyn
# Output: Mach-O 64-bit executable x86_64
```

**Why it works:**
- macOS includes both ARM64 and x86_64 SDKs
- Clang can target either architecture
- No additional tools needed

### Linux ARM64 (from x86_64)

```bash
# On Linux x86_64 runner
sudo apt-get install -y gcc-aarch64-linux-gnu clang

clang --target=aarch64-linux-gnu \
  -Wall -Wextra -std=c11 -O2 -DCOMPILER_MAIN \
  -o build/wyn src/wyn.c src/compiler.c

# Produces ARM64 binary
file build/wyn
# Output: ELF 64-bit LSB executable, ARM aarch64
```

**Why it works:**
- LLVM can generate ARM64 code on x86_64
- gcc-aarch64-linux-gnu provides ARM64 system libraries
- Standard cross-compilation toolchain

## Workflow Structure

### release.yml

```yaml
jobs:
  # Native builds (3 platforms)
  build-release:
    strategy:
      matrix:
        - macOS ARM64 (macos-14)
        - Linux x86_64 (ubuntu-22.04)
        - Windows x86_64 (windows-2022)
  
  # Cross-compile macOS x86_64
  cross-compile-macos-x86:
    runs-on: macos-14
    steps:
      - Cross-compile with clang -target
  
  # Cross-compile Linux ARM64
  cross-compile-linux-arm:
    runs-on: ubuntu-22.04
    steps:
      - Install cross-compilation tools
      - Cross-compile with clang --target
  
  # Create release (waits for all builds)
  create-release:
    needs: [build-release, cross-compile-macos-x86, cross-compile-linux-arm]
```

## Benefits

### Speed
- **Before:** 20+ hours (waiting for rare runners)
- **After:** ~10 minutes (all builds in parallel)

### Reliability
- **Before:** Dependent on rare runner availability
- **After:** Uses only common, readily available runners

### Cost
- **Before:** Same (waiting doesn't cost, but blocks releases)
- **After:** Same compute time, much faster wall-clock time

## Validation

### Binary Verification

Each cross-compiled binary is verified:

```bash
# Check architecture
file build/wyn

# Test execution (if possible)
./build/wyn version
```

### Testing Strategy

- **Native builds:** Run full test suite (54 tests)
- **Cross-compiled:** Build verification only (can't run on host architecture)
- **Post-release:** Community testing on actual hardware

## Limitations

### Cannot Run Tests

Cross-compiled binaries can't run on the host:
- x86_64 binary won't run on ARM64 macOS (without Rosetta)
- ARM64 binary won't run on x86_64 Linux (without emulation)

**Mitigation:**
- Native builds run full test suite
- Cross-compiled builds are verified with `file` command
- Community testing after release

### Requires Cross-Compilation Tools

**macOS x86_64:**
- ✅ No additional tools needed (built into Xcode)

**Linux ARM64:**
- ⚠️ Requires `gcc-aarch64-linux-gnu` package
- ✅ Installed automatically in workflow

## Alternative Approaches Considered

### 1. Wait for Runners
- ❌ 20+ hour delays unacceptable
- ❌ Blocks releases
- ❌ Poor user experience

### 2. Self-Hosted Runners
- ❌ Requires infrastructure
- ❌ Maintenance overhead
- ❌ Security concerns

### 3. Cross-Compilation (CHOSEN)
- ✅ Fast (minutes instead of hours)
- ✅ No infrastructure needed
- ✅ Uses LLVM's native capabilities
- ✅ Reliable and repeatable

### 4. Skip Platforms
- ❌ Reduces platform support
- ❌ Poor user experience
- ❌ Against project goals

## Future Improvements

### Short Term
- Add smoke tests for cross-compiled binaries using emulation (QEMU)
- Cache cross-compilation toolchains

### Long Term
- Self-hosted ARM64 runners (if needed)
- Docker-based cross-compilation
- Universal binaries for macOS (fat binaries with both architectures)

## Testing Cross-Compilation Locally

### macOS x86_64 (on ARM64 Mac)

```bash
clang -target x86_64-apple-macos11.0 -Wall -Wextra -std=c11 -O2 \
  -DCOMPILER_MAIN -o build/wyn-x86_64 src/wyn.c src/compiler.c

file build/wyn-x86_64
# Should show: Mach-O 64-bit executable x86_64

# Test with Rosetta
./build/wyn-x86_64 version
```

### Linux ARM64 (on x86_64 Linux)

```bash
sudo apt-get install -y gcc-aarch64-linux-gnu clang

clang --target=aarch64-linux-gnu -Wall -Wextra -std=c11 -O2 \
  -DCOMPILER_MAIN -o build/wyn-arm64 src/wyn.c src/compiler.c

file build/wyn-arm64
# Should show: ELF 64-bit LSB executable, ARM aarch64

# Test with QEMU (if installed)
sudo apt-get install -y qemu-user-static
qemu-aarch64-static build/wyn-arm64 version
```

## Conclusion

Cross-compilation solves the runner availability problem while maintaining full platform support. LLVM's cross-compilation capabilities make this straightforward and reliable.

**Result:** All 5 platforms build in ~10 minutes instead of 20+ hours.

---

**Implementation Date:** 2025-12-21  
**Status:** Production  
**Platforms:** 5 (3 native, 2 cross-compiled)  
**Build Time:** ~10 minutes (down from 20+ hours)
