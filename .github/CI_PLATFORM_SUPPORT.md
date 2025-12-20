# CI/CD Platform Support

This document describes Wyn's multi-platform CI/CD strategy and build matrix.

## Platform Matrix

### Tier 1: Full CI/CD (Complete Test Suite)

These platforms run the full test suite (54 tests) on every commit:

| Platform | OS | Architecture | Runner | Status |
|----------|----|--------------| -------|--------|
| **macOS ARM64** | macOS 14 | Apple Silicon | `macos-14` | ✅ Primary |
| **Linux x86_64** | Ubuntu 22.04 | x86_64 | `ubuntu-22.04` | ✅ Full |
| **Windows x86_64** | Windows Server 2022 | x86_64 | `windows-2022` | ⚠️ Smoke tests |

### Tier 2: Build + Smoke Tests

These platforms build successfully and run basic smoke tests:

| Platform | OS | Architecture | Runner | Status |
|----------|----|--------------| -------|--------|
| **macOS x86_64** | macOS 13 | Intel | `macos-13` | ✅ Build |
| **Linux ARM64** | Ubuntu 22.04 | aarch64 | `ubuntu-22.04-arm64` | ✅ Build |

## LLVM Installation

Each platform uses its native package manager for LLVM 18:

### macOS
```bash
brew install llvm@18
```
- ARM64 path: `/opt/homebrew/opt/llvm`
- x86_64 path: `/usr/local/opt/llvm`

### Linux
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
```
- Path: `/usr/lib/llvm-18`

### Windows
```powershell
choco install llvm --version=18
```
- Path: `C:\Program Files\LLVM`

## Build Process

### Unix (macOS, Linux)
```bash
make clean
make LLVM_PREFIX=/path/to/llvm
./build/wyn --version
```

### Windows
```bash
mkdir -p build
clang -Wall -Wextra -std=c11 -O2 -DCOMPILER_MAIN -D_WIN32 \
  -o build/wyn.exe src/wyn.c src/compiler.c
./build/wyn.exe --version
```

## Test Strategy

### Full Test Suite (Tier 1)
- Runs all 54 tests using `./build/wyn test`
- Parallel execution with smart caching
- Tests extension methods, method syntax, stdlib

### Smoke Tests (All Platforms)
1. **Basic compilation**: `print("Hello, Wyn!")`
2. **Method syntax**: `"hello".upper()`
3. **Extension methods**: `fn int.squared(self)`
4. **Examples**: Run 3-5 example programs

## Artifacts

Each successful build produces a platform-specific artifact:

```
wyn-<version>-<platform>.tar.gz
├── release/
│   ├── wyn (or wyn.exe)
│   ├── std/
│   ├── examples/
│   ├── editors/
│   ├── README.md
│   ├── INSTALL.md
│   └── LICENSE
```

### Artifact Naming
- `wyn-v0.3.0-macos-arm64.tar.gz`
- `wyn-v0.3.0-macos-x86_64.tar.gz`
- `wyn-v0.3.0-linux-x86_64.tar.gz`
- `wyn-v0.3.0-linux-arm64.tar.gz`
- `wyn-v0.3.0-windows-x86_64.tar.gz`

## Workflows

### 1. CI Workflow (`.github/workflows/ci.yml`)
**Triggers:**
- Push to `main` or `develop`
- Pull requests to `main` or `develop`
- Manual dispatch

**Jobs:**
- `tier1-platforms`: Full tests on macOS ARM64, Linux x86_64, Windows x86_64
- `tier2-platforms`: Build + smoke tests on macOS x86_64, Linux ARM64
- `ci-summary`: Aggregate results

**Artifacts:** Retained for 30 days

### 2. Release Workflow (`.github/workflows/release.yml`)
**Triggers:**
- Push tags matching `v*` (e.g., `v0.3.0`)
- Manual dispatch with version input

**Jobs:**
- `build-release`: Build all 5 platforms
- `create-release`: Create GitHub release with all binaries

**Artifacts:** Retained for 90 days + attached to GitHub release

### 3. Legacy Build Workflow (`.github/workflows/build.yml`)
**Status:** Deprecated, replaced by `ci.yml`

## Platform-Specific Notes

### macOS
- ✅ Full support on both ARM64 and x86_64
- ✅ All 54 tests passing
- ✅ Native LLVM via Homebrew
- ⚠️ Requires macOS 11.0+ deployment target

### Linux
- ✅ Full support on x86_64
- ✅ ARM64 builds successfully
- ✅ Native LLVM via apt
- ⚠️ Requires `-fPIC` flag for shared libraries

### Windows
- ⚠️ Experimental support
- ⚠️ Smoke tests only (full test suite pending)
- ⚠️ Uses `clang` directly (no Makefile support yet)
- ⚠️ Known issues:
  - `fork()` not available (test runner needs rewrite)
  - `find` command not available (needs Windows alternative)
  - Path separator differences (\ vs /)

## Known Issues

### Windows Test Runner
The current test runner (`wyn test`) uses Unix-specific features:
- `fork()` for parallel execution
- `find` command for test discovery

**Solution:** Rewrite test runner to use:
- Windows: `CreateProcess` or threads
- Test discovery: C's `opendir/readdir` (cross-platform)

### LLVM Path Detection
The Makefile now auto-detects LLVM paths, but may need manual override:
```bash
make LLVM_PREFIX=/custom/path/to/llvm
```

## Future Improvements

### Short Term
1. ✅ Multi-platform CI/CD (DONE)
2. ⏳ Windows full test suite support
3. ⏳ Cross-platform test runner (no fork/find)
4. ⏳ Windows Makefile support

### Long Term
1. ⏳ Windows ARM64 support
2. ⏳ FreeBSD support
3. ⏳ Cross-compilation support
4. ⏳ Docker images for all platforms

## Debugging CI Failures

### LLVM Not Found
```bash
# Check LLVM installation
llvm-config --version

# Verify path
ls -la /opt/homebrew/opt/llvm/bin  # macOS ARM64
ls -la /usr/lib/llvm-18/bin        # Linux
```

### Build Failures
```bash
# Clean build
make clean
make LLVM_PREFIX=/path/to/llvm

# Verbose output
make V=1
```

### Test Failures
```bash
# Run single test
./build/wyn run tests/specific_test.wyn

# Run with verbose output
./build/wyn test --verbose  # (if implemented)
```

## Contributing

When adding new platforms:

1. Add to CI matrix in `.github/workflows/ci.yml`
2. Add LLVM installation steps
3. Add platform-specific build commands
4. Update this documentation
5. Test locally if possible
6. Start with smoke tests, expand to full tests

## Resources

- **GitHub Actions Runners**: https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners
- **LLVM Installation**: https://apt.llvm.org/
- **Platform Detection Script**: `scripts/detect_platform.sh`

---

**Last Updated:** 2025-12-20  
**Status:** Production Ready (Unix), Experimental (Windows)
