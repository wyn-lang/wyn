# Testing CI/CD Locally

## Quick Test (Recommended)

Run the CI steps manually:
```bash
./scripts/validate_ci.sh
```

This runs:
- Clean build
- Compile
- Run 54 tests
- Test examples
- Test REPL

## Test with Docker (Linux Build)

Test the Linux build in a container:
```bash
./scripts/test_linux_build.sh
```

This:
- Creates Ubuntu 22.04 container
- Installs LLVM 14
- Builds Wyn
- Runs all tests
- Tests examples

## Test with Act (Full GitHub Actions)

Run the actual GitHub Actions workflow locally:
```bash
# Install act
brew install act

# Run all jobs
./scripts/test_ci_locally.sh

# Or run specific job
act -j build-macos
act -j build-linux
```

## Comparison

| Method | Speed | Accuracy | Requirements |
|--------|-------|----------|--------------|
| validate_ci.sh | ⚡ Fast | Good | None |
| Docker | 🐢 Slow | Better | Docker |
| act | 🐌 Slowest | Best | Docker + act |

## Recommendation

1. **Development**: Use `validate_ci.sh` (fast, good enough)
2. **Before Push**: Use Docker test (catches Linux issues)
3. **Final Check**: Use act (exact GitHub Actions simulation)

## Usage

```bash
# Quick validation (30 seconds)
./scripts/validate_ci.sh

# Linux build test (2-3 minutes)
./scripts/test_linux_build.sh

# Full CI/CD test (5-10 minutes)
./scripts/test_ci_locally.sh
```
