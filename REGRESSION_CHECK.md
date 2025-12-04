# Regression Checklist

## Build System
- [x] `make all` - Builds compiler, runtimes, CLI
- [x] `make test` - Runs test suite (37/72 tests)
- [x] `make bench` - Runs benchmarks
- [x] `make install` - Installs to /usr/local/bin
- [x] `make clean` - Cleans build artifacts

## CLI Commands
- [x] `wyn version` - Shows version
- [x] `wyn new <name>` - Creates project
- [x] `wyn build` - Builds project
- [x] `wyn run` - Builds and runs
- [x] `wyn exec <file>` - Compiles and runs file
- [x] `wyn exec` (stdin) - Reads from stdin
- [x] `wyn inline <code>` - Executes code inline
- [x] `wyn test` - Runs tests
- [x] `wyn bench` - Runs benchmarks
- [x] `wyn clean` - Cleans project

## Compiler
- [x] Basic compilation works
- [x] Stage 1 type checking (`--stage1-tc`)
- [x] Stage 1 optimizations (`--stage1-opt`)
- [x] Quiet mode (`-q`)
- [x] Examples compile

## Standard Library
- [x] 500+ functions implemented
- [x] Time functions work (time_now, sleep)
- [x] I/O functions work (read_file, write_file)
- [x] Math functions work (sqrt, sin, cos, log)
- [x] String functions work (split, find, concat)

## Cross-Platform
- [x] No bash scripts (except iOS-specific)
- [x] CLI written in Wyn
- [x] Test runner written in Wyn
- [x] Benchmark runner written in Wyn

## Known Issues

### Test Runner
⚠️ **Only runs 37/72 tests** - Hardcoded test list incomplete
- Missing: All example tests (34 tests)
- Missing: atomic_spawn_test
- Fix: Update tests/_run_tests.wyn with complete list

### GitHub Actions
⚠️ **May still reference old bash scripts**
- Check: .github/workflows/build.yml
- Verify: Uses new Wyn-based tools

### Documentation
⚠️ **May have outdated references**
- Check: References to install.sh
- Check: References to bash scripts
- Check: Platform support accuracy

## Recommendations

### High Priority
1. Fix test runner to run all 72 tests
2. Verify GitHub Actions work with new structure
3. Update any docs referencing removed bash scripts

### Medium Priority
1. Add more CLI commands (format, lint, doc)
2. Improve error messages in CLI
3. Add progress indicators

### Low Priority
1. Optimize test runner performance
2. Add colored output to CLI
3. Add shell completion

## Testing Checklist

Run these manually to verify:
```bash
# Build
make clean && make all

# Tests
make test

# Benchmarks
make bench

# CLI
./build/wyn version
./build/wyn new testapp
cd testapp && ../build/wyn run .

# Exec
echo "print(42)" | ./build/wyn exec
./build/wyn inline 'print(123)'

# Install
sudo make install
wyn version
```

## Status

**Overall**: Good, but test runner needs fixing to run all tests.
