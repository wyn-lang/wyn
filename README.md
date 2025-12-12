# Wyn Programming Language

Fast, compiled systems language with clean, expressive syntax, powered by LLVM.

## What is Wyn?

Wyn is a **production-ready infrastructure/DevOps language** that combines:
- **Speed:** LLVM-powered native code generation
- **Simplicity:** Clean, expressive syntax
- **Multi-platform:** ARM64, x86_64, and more via LLVM
- **Concurrency:** Go-style green threads (1.3M tasks/sec)
- **Complete Stdlib:** 99 infrastructure functions, ALL REAL

## 🎉 Production Ready

The language is **production-ready** with **100% passing test suite**:
- ✅ All core language features
- ✅ 11 standard library modules (99 functions)
- ✅ HTTP client and server
- ✅ Concurrency (spawn, channels)
- ✅ String arrays with indexing
- ✅ Pattern matching
- ✅ Developer tools (REPL, LSP, formatter)

## Test Suite Status

- **Total**: 109/109 passing (100%)
- **Examples**: 82/82
- **Tests**: 27/27

## Quick Start

```bash
# 1. Build compiler
make

# 2. Run tests (100% pass)
make test

# 3. Try the REPL
./build/wyn-repl

# 4. Write a program
cat > hello.wyn << 'EOF'
fn main() {
    print("Hello, Wyn!")
}
EOF

# 5. Compile and run
./build/wync hello.wyn
./a.out
```

## Example Programs

### HTTP Server
```wyn
import net

fn main() {
    const server: int = net.server_create(8080)
    print("Server on :8080")
    
    const client: int = net.server_accept(server)
    const path: str = net.parse_path(net.server_read_request(client))
    
    if path == "/api" {
        net.server_send_json(client, 200, "{status:ok}")
    } else {
        net.server_send_response(client, 200, "Hello!")
    }
    
    net.server_close_client(client)
    net.server_close(server)
}
```

### File Processing
```wyn
import io
import regex

fn main() {
    const log: str = io.read("/var/log/app.log")
    const errors: int = regex.count(log, "ERROR")
    print("Errors found:")
    print(errors)
}
```

### Data Structures
```wyn
import collections

fn main() {
    const map: str = collections.hashmap_new()
    collections.hashmap_put(map, "name", "Alice")
    collections.hashmap_put(map, "role", "admin")
    
    const name: str = collections.hashmap_get(map, "name")
    print("User: " + name)
}
```

## Standard Library

### Complete Modules (99 functions, ALL REAL):

- **io** (13) - File operations, directories
- **os** (8) - Process, environment, system
- **net** (23) - HTTP client/server, TCP, UDP
- **json** (5) - Parsing, stringify
- **time** (7) - Timestamps, formatting, sleep
- **log** (5) - Structured logging
- **encoding** (6) - Base64, hex, URL
- **hash** (3) - SHA256, MD5, SHA1
- **compress** (7) - Gzip, tar
- **regex** (4) - Pattern matching
- **collections** (13) - HashMap, Set
- **crypto** (5) - AES-256, HMAC, random

Plus: **string**, **array**, **math**, **task**, **core**, **result**

See [API Reference](docs/API_REFERENCE.md) for complete documentation.

## Developer Tools

### REPL - Interactive Shell
```bash
./build/wyn-repl
wyn> 5 + 3
8
wyn> "Hello"
Hello
```

### LSP Server - IDE Support
```bash
./scripts/install_lsp.sh
```

Works with VS Code, Neovim, Emacs, Sublime Text.

See [LSP Setup Guide](docs/LSP_SETUP.md).

### Code Formatter
```bash
./scripts/wyn_fmt.sh program.wyn
```

### Beautiful Error Messages
```
╭─ Error in file.wyn:3:5
│
│  type mismatch in let: expected str, got int
│
╰─ Type error

   💡 Use int_to_str() to convert int to string
```

## Building

```bash
make          # Build compiler
make test     # Run tests (100% pass)
make clean    # Clean build
```

## IDE Setup

### VS Code
```bash
cp -r editors/vscode ~/.vscode/extensions/wyn-language
cd ~/.vscode/extensions/wyn-language && npm install
```

### Neovim
```bash
cp editors/nvim/wyn.lua ~/.config/nvim/lua/
# Add require('wyn') to init.lua
```

See [docs/LSP_SETUP.md](docs/LSP_SETUP.md) for all editors.

## Documentation

- [Quick Reference](docs/QUICK_REFERENCE.md) - Language syntax
- [API Reference](docs/API_REFERENCE.md) - All 99 functions
- [LSP Setup](docs/LSP_SETUP.md) - IDE integration
- [Net API](docs/NET_API.md) - Networking guide
- [Examples](examples/) - 82 working examples
- [Status](STATUS.md) - Current capabilities

## Performance

- **Compilation**: 0.3-0.5s
- **Concurrency**: 1.3M tasks/sec
- **Memory**: 8KB per task
- **Startup**: <100ms

## Use Cases

Wyn excels at:
- System automation
- DevOps tooling
- Infrastructure monitoring
- API services
- Log processing
- Configuration management
- Build tools
- Microservices
- CLI applications

## Status

| Component | Status |
|-----------|--------|
| Core Language | ✅ 100% Complete |
| LLVM Backend | ✅ 100% Working |
| Standard Library | ✅ 11 Modules (99 functions) |
| Test Suite | ✅ 100% Passing (109/109) |
| Developer Tools | ✅ REPL, LSP, Formatter |
| Documentation | ✅ Complete |

## Contributing

See [TODO.md](TODO.md) for planned features.

## License

See LICENSE file.

## Where does the name come from?

In Old English, wyn (or wynn) primarily meant joy, delight, pleasure, or bliss, stemming from the Proto-Germanic *wunjō; it was also the name of the runic letter (ƿ) that represented the 'w' sound.
