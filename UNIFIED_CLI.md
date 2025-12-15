# Unified Wyn CLI

## Summary

Consolidated `wync` and `wyn-repl` into a single `wyn` binary with subcommands.

## Changes

### Before
- `./build/wync` - Compiler binary
- `./build/wyn-repl` - REPL binary
- Separate binaries for each tool

### After
- `./build/wyn` - Single unified binary
- All functionality through subcommands

## Commands

```bash
./build/wyn compile <file>    # Compile a .wyn file
./build/wyn run <file>         # Compile and run
./build/wyn repl               # Interactive REPL
./build/wyn fmt <file>         # Format/validate code
./build/wyn doc [module]       # Show documentation
./build/wyn lsp                # Start LSP server
./build/wyn pkg <cmd>          # Package manager (stub)
./build/wyn test               # Run test suite
./build/wyn version            # Show version
./build/wyn help               # Show help
```

## Implementation

### Files Modified
1. **src/wyn.c** (new) - Main CLI entry point
2. **bootstrap/stage0.c** - Renamed `main()` to `compiler_main()` with `#ifdef COMPILER_MAIN`
3. **Makefile** - Updated to build unified binary

### Architecture
- `src/wyn.c` contains the CLI router and subcommand implementations
- Compiler functionality is linked from `bootstrap/stage0.c`
- REPL and LSP are implemented inline in `wyn.c`
- Package manager is a stub for future implementation

## Benefits

1. **Simpler UX** - One binary, consistent interface
2. **Easier distribution** - Single file to install
3. **Unified help** - All commands documented in one place
4. **Extensible** - Easy to add new subcommands
5. **Standard CLI pattern** - Follows conventions (git, cargo, npm, etc.)

## Build

```bash
make clean
make
```

Produces: `./build/wyn` (~200KB)

## Testing

All existing tests pass with the new binary:

```bash
make test
```

## Future Additions

Easy to add new subcommands:
- `wyn bench` - Run benchmarks
- `wyn init <name>` - Create new project
- `wyn build` - Build project
- `wyn check` - Type check without compiling
- `wyn clean` - Clean build artifacts

## Migration

No backward compatibility needed (single user).

Old commands map to:
- `wync file.wyn` → `wyn compile file.wyn`
- `wyn-repl` → `wyn repl`
