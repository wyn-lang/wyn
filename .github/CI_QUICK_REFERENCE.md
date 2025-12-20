# CI/CD Quick Reference

## Local Testing Before Push

### Test on Current Platform
```bash
# Full build and test
make clean
make
make test

# Smoke tests
./build/wyn run examples/hello.wyn
./build/wyn run examples/extension_methods.wyn
```

### Verify LLVM Setup
```bash
# Check platform and LLVM
./scripts/detect_platform.sh info

# Manual LLVM path
make LLVM_PREFIX=/custom/llvm/path
```

### Test Specific Platform Behavior
```bash
# macOS ARM64 (default on M1+)
make clean && make

# macOS x86_64 (Intel)
arch -x86_64 make clean && arch -x86_64 make

# Linux (in Docker)
docker run --rm -v $(pwd):/wyn -w /wyn ubuntu:22.04 bash -c "
  apt-get update && apt-get install -y build-essential wget
  wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18
  make LLVM_PREFIX=/usr/lib/llvm-18
"
```

## GitHub Actions

### Trigger CI Manually
```bash
# Via GitHub CLI
gh workflow run ci.yml

# Via web interface
# Go to Actions tab → CI → Run workflow
```

### Check CI Status
```bash
# List recent runs
gh run list --workflow=ci.yml

# Watch specific run
gh run watch <run-id>

# View logs
gh run view <run-id> --log
```

### Download Artifacts
```bash
# List artifacts from latest run
gh run list --workflow=ci.yml --limit 1
gh run download <run-id>

# Download specific artifact
gh run download <run-id> -n wyn-macos-arm64
```

## Release Process

### Create Release
```bash
# Tag and push
git tag v0.3.1
git push origin v0.3.1

# This triggers release workflow automatically
```

### Manual Release Trigger
```bash
# Via GitHub CLI
gh workflow run release.yml -f version=v0.3.1

# Via web interface
# Go to Actions tab → Release → Run workflow → Enter version
```

### Verify Release
```bash
# Check release status
gh release view v0.3.1

# Download release assets
gh release download v0.3.1

# List all releases
gh release list
```

## Platform-Specific Commands

### macOS
```bash
# Install dependencies
brew install llvm@18

# Build
make LLVM_PREFIX=$(brew --prefix llvm@18)

# Test
./build/wyn test
```

### Linux
```bash
# Install dependencies
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
sudo apt-get install -y clang-18

# Build
make LLVM_PREFIX=/usr/lib/llvm-18

# Test
./build/wyn test
```

### Windows (Git Bash / MSYS2)
```bash
# Install dependencies
choco install llvm --version=18

# Build (direct clang, Makefile not fully supported yet)
mkdir -p build
clang -Wall -Wextra -std=c11 -O2 -DCOMPILER_MAIN -D_WIN32 \
  -o build/wyn.exe src/wyn.c src/compiler.c

# Test (smoke tests only)
./build/wyn.exe run examples/hello.wyn
```

## Troubleshooting

### CI Failing on Specific Platform
```bash
# 1. Check workflow logs
gh run view <run-id> --log

# 2. Reproduce locally (if possible)
# See platform-specific commands above

# 3. Check LLVM installation
llvm-config --version
which clang

# 4. Verify platform detection
./scripts/detect_platform.sh info
```

### Artifact Upload Failures
```bash
# Check artifact size (max 2GB per artifact)
du -sh release/

# Verify artifact name is unique
# Each platform must have unique artifact_name in workflow
```

### Release Creation Failures
```bash
# Check permissions
# Workflow needs: permissions: contents: write

# Verify tag format
git tag -l "v*"

# Check if release already exists
gh release view v0.3.1
```

## Workflow Files

- **CI**: `.github/workflows/ci.yml` - Multi-platform build and test
- **Release**: `.github/workflows/release.yml` - Tagged release builds
- **Legacy**: `.github/workflows/build.yml` - Deprecated, kept for reference

## Environment Variables

### CI Workflow
```yaml
LLVM_VERSION: "18"  # LLVM version to install
```

### Build Process
```bash
LLVM_PREFIX=/path/to/llvm  # Override LLVM location
CC=clang                    # Override C compiler
CFLAGS="-O3"               # Override compiler flags
```

## Useful GitHub CLI Commands

```bash
# Install GitHub CLI
brew install gh  # macOS
apt install gh   # Linux

# Authenticate
gh auth login

# Watch CI in real-time
gh run watch

# Cancel running workflow
gh run cancel <run-id>

# Re-run failed jobs
gh run rerun <run-id> --failed

# View workflow definition
gh workflow view ci.yml
```

## Best Practices

1. **Always test locally first** before pushing
2. **Use draft PRs** for experimental changes
3. **Check CI status** before merging
4. **Tag releases** only after CI passes on main
5. **Document platform-specific issues** in PR description
6. **Use `continue-on-error: true`** for experimental platforms

## Quick Links

- [CI Workflow](.github/workflows/ci.yml)
- [Release Workflow](.github/workflows/release.yml)
- [Platform Support Doc](.github/CI_PLATFORM_SUPPORT.md)
- [GitHub Actions Docs](https://docs.github.com/en/actions)

---

**Last Updated:** 2025-12-20
