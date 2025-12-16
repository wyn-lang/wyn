# Wyn Language - Vision & Roadmap

**Last Updated:** December 16, 2025
**Progress:** 45% complete

## Mission Statement

**"The universal DevOps language - replace bash/Python/PowerShell with one compiled, cross-platform tool"**

### The Problem
DevOps is a mess of different scripting languages:
- Bash/Shell (Linux/macOS only, cryptic syntax)
- PowerShell (Windows-centric, verbose)
- Python (slow startup, dependency hell)
- Node.js (heavy runtime, callback hell)
- Go (verbose for simple scripts, compilation required)

### The Solution
**Wyn:** Simple like Python, fast like Go, works everywhere.

```wyn
// One script, all platforms
import os, io, net

fn deploy() {
    const env = os.getenv("ENV")
    const config = io.read("config.json")
    
    if env == "prod" {
        net.http_post("https://api.deploy.com", config)
    }
}
```

**Tagline:** "Stop writing bash, Python, AND PowerShell. Write Wyn."

## Core Principles

1. **Cross-Platform First** - Identical behavior on Linux, macOS, Windows
2. **Fast Startup** - Compiled binaries, instant execution
3. **Simple Syntax** - Python-like, not C-like
4. **DevOps-Focused** - Stdlib designed for infrastructure tasks
5. **Script or Compile** - `wyn run script.wyn` or `wyn build script.wyn`

## Competitive Positioning

| Feature | Bash | Python | PowerShell | Go | **Wyn** |
|---------|------|--------|------------|----|----|
| Cross-platform | ❌ | ✅ | Windows-only | ✅ | ✅ |
| Fast startup | ✅ | ❌ | ❌ | ✅ | ✅ |
| Compiled | ❌ | ❌ | ❌ | ✅ | ✅ |
| Easy syntax | ❌ | ✅ | ❌ | ⚠️ | ✅ |
| Concurrency | ❌ | ⚠️ | ⚠️ | ✅ | ✅ |
| Single binary | ✅ | ❌ | ✅ | ✅ | ✅ |
| Stdlib for DevOps | ⚠️ | ✅ | ✅ | ⚠️ | ✅ |

## Critical Design Issues to Fix

### 1. Type System is Too Weak
**Current:**
```wyn
const x: int = 5
const y: str = "hello"
```

**Missing:**
- Proper error handling (Result<T, E>)
- Optional types (Option<T>)
- Generic collections (Array<T>, Map<K,V>)
- User-defined types with methods

**Priority:** HIGH - Finish generics implementation

### 2. No Real Error Handling
**Current:** Functions fail silently or crash

**Need:**
```wyn
fn read_file(path: str) -> Result<str, Error> {
    // Explicit error handling
}
```

**Priority:** HIGH - Add Result/Option types

### 3. String/Array Handling is Primitive
**Current:** No slicing, limited operations

**Need:**
```wyn
const arr = ["a", "b", "c"]
arr[1:3]           // Slicing
arr.map(fn)        // Functional operations
arr.filter(fn)     // Filtering
```

**Priority:** MEDIUM - Add slicing and functional APIs

### 4. No Package System
**Current:** Everything in std/, no code sharing

**Need:**
```wyn
import github.com/user/package
```

**Priority:** HIGH - Design and implement package manager

### 5. Concurrency is Half-Baked
**Current:** spawn works, but no channels, no async/await

**Need:** Finish M:N threading or switch to async/await

**Priority:** MEDIUM - Complete concurrency primitives

### 6. Standard Library Gaps
**Missing:**
- Database drivers (postgres, mysql, sqlite)
- HTTP server framework (not just raw sockets)
- Testing framework
- Serialization (protobuf, msgpack)
- CLI argument parsing
- Environment config management

**Priority:** MEDIUM - Add based on use cases

## Design Decisions

### Memory Model
**Decision:** Explicit GC with escape hatches for manual control
- Keep GC for simplicity
- Add manual memory management for performance-critical code

### Error Philosophy
**Decision:** Result types (like Rust/Go)
```wyn
const result = risky_operation()
if result.is_err() {
    handle(result.err())
}
```

**Rationale:** More explicit, better for infrastructure code

### Type Annotations
**Decision:** Required for function signatures, optional for locals
```wyn
fn add(a: int, b: int) -> int {  // Required
    const sum = a + b             // Inferred
    return sum
}
```

### Compilation Model
**Decision:** Module system with incremental compilation
- Need build tool (like cargo/go build)
- Support both script mode and compiled mode

## 6-Month Roadmap

### Month 1-2: Cross-Platform Foundation ⚠️ CRITICAL
- [ ] Test all 99 stdlib functions on Windows
- [ ] Fix path separators, line endings
- [ ] PowerShell-style commands
- [ ] CI/CD for all platforms

**Goal:** Wyn works identically on Linux, macOS, Windows

### Month 3: Shell Replacement Features
- [ ] Command builder
  ```wyn
  os.command("docker")
    .arg("run")
    .arg("-d")
    .arg("nginx")
    .output()
  ```
- [ ] Better pipe operator
- [ ] Glob patterns: `io.glob("**/*.wyn")`
- [ ] Process management: spawn, wait, kill

**Goal:** Replace bash for common tasks

### Month 4: Python Replacement Features
- [ ] More file operations: copy, move, chmod, chown
- [ ] Method chaining: `"hello".upper().split("").join("")`
- [ ] List operations: filter, map, reduce
- [ ] Dict/Map literals: `{"key": "value"}`

**Goal:** Replace Python for scripting

### Month 5: Go Replacement Features
- [ ] Finish channels implementation
- [ ] Add context/cancellation
- [ ] HTTP framework (not just raw server)
- [ ] Database drivers (postgres, mysql, sqlite)

**Goal:** Replace Go for services

### Month 6: Developer Experience
- [ ] Package manager: `wyn pkg install`
- [ ] Project scaffolding: `wyn new`, `wyn init`
- [ ] Better error messages
- [ ] Debugger support
- [ ] Profiler

**Goal:** Professional development experience

## Killer Features for DevOps

### 1. Shebang Support ✅ DONE
```wyn
#!/usr/bin/env wyn run
import os

fn main() {
    print("Hello from Wyn script!")
}
```

Make `.wyn` files executable like Python/bash.

### 2. Embedded Mode ✅ DONE
```bash
# Run Wyn inline
wyn -e 'print(os.exec_output("uptime"))'
```

### 3. Enhanced REPL ✅ DONE
```bash
$ wyn repl
wyn> import os
wyn> os.hostname()
"my-machine"
wyn> const files = io.list_files(".")
wyn> files.len()
42
```

### 4. Script Caching ⚠️ TODO
```bash
# First run: compiles
$ wyn run deploy.wyn
Compiling... (0.3s)
Deploying...

# Second run: uses cache
$ wyn run deploy.wyn
Deploying...  # Instant!
```

### 5. Cross-Compilation ⚠️ TODO
```bash
# Build for all platforms
$ wyn build --target linux-amd64 deploy.wyn
$ wyn build --target windows-amd64 deploy.wyn
$ wyn build --target darwin-arm64 deploy.wyn
```

## Shell Replacement Checklist

What bash does well, Wyn must do better:

- [x] Command execution: `os.exec()`
- [x] Command output: `os.exec_output()`
- [x] Environment variables: `os.getenv()`, `os.setenv()`
- [x] Exit codes: `os.exit()`
- [ ] Pipes: Improve `|>` operator
- [ ] Redirects: stdout, stderr, stdin
- [ ] Job control: background processes
- [ ] Glob patterns: `*.txt`, `**/*.wyn`
- [ ] Command substitution: `$(command)`
- [ ] Here documents
- [ ] Process substitution

## Python Replacement Checklist

What Python does well, Wyn must match:

- [x] HTTP requests: `net.http_get()`
- [x] JSON parsing: `json.parse()`
- [x] File I/O: `io.read()`, `io.write()`
- [x] String operations: split, replace, etc.
- [ ] File operations: copy, move, chmod
- [ ] Method chaining
- [ ] List comprehensions (or equivalent)
- [ ] Dict/Map literals
- [ ] Context managers (or equivalent)
- [ ] Decorators (or equivalent)
- [ ] Virtual environments (package isolation)

## Go Replacement Checklist

What Go does well, Wyn must match:

- [x] Goroutines: `spawn {}`
- [x] HTTP server: `net.server_create()`
- [ ] Channels: `channel.new()`, `send()`, `recv()`
- [ ] Context/cancellation
- [ ] HTTP framework (routing, middleware)
- [ ] Database drivers
- [ ] Testing framework
- [ ] Benchmarking
- [ ] Profiling

## Standard Library Priorities

### Tier 1: Essential ✅ COMPLETE
- io, os, net, json, time, log, encoding, hash, compress, regex, collections, crypto, cli, array

### Tier 2: DevOps Critical ✅ MOSTLY COMPLETE
- [x] **http** - Framework with routing ✅
- [x] **testing** - Testing framework ✅
- [x] **cli** - Argument parsing ✅
- [x] **database** - SQLite driver ✅
- [ ] **config** - YAML, TOML parsing (partial)
- [ ] **template** - Text templating
- [ ] **ssh** - SSH client/server
- [ ] **git** - Git operations

### Tier 3: Core Utilities
- [ ] **email** - SMTP client
- [ ] **xml** - XML parsing
- [ ] **csv** - CSV parsing
- [ ] **markdown** - Markdown parsing

## Package Ecosystem (Not Core Stdlib)

**Philosophy:** Keep core lean. Cloud SDKs, databases, and platform-specific tools should be packages.

### Tier 1: Essential Packages (First 6 months)
- [ ] **aws** - AWS SDK (S3, EC2, Lambda, DynamoDB, etc.)
- [ ] **postgres** - PostgreSQL driver
- [ ] **mysql** - MySQL driver  
- [ ] **sqlite** - SQLite driver
- [ ] **docker** - Docker API client
- [ ] **k8s** - Kubernetes client
- [ ] **redis** - Redis client

**Example usage:**
```bash
wyn pkg install aws
wyn pkg install postgres
```

```wyn
import aws        // From package
import postgres   // From package

fn main() {
    const s3 = aws.s3("us-east-1")
    s3.upload("bucket", "file.txt", "data")
    
    const db = postgres.connect("localhost:5432")
    db.query("SELECT * FROM users")
}
```

### Tier 2: Cloud & Infrastructure Packages
- [ ] **gcp** - Google Cloud SDK
- [ ] **azure** - Azure SDK
- [ ] **terraform** - Terraform provider
- [ ] **ansible** - Ansible integration
- [ ] **prometheus** - Prometheus client
- [ ] **grafana** - Grafana API

### Tier 3: Specialized Packages
- [ ] **mongodb** - MongoDB driver
- [ ] **cassandra** - Cassandra driver
- [ ] **elasticsearch** - Elasticsearch client
- [ ] **kafka** - Kafka client
- [ ] **rabbitmq** - RabbitMQ client
- [ ] **grpc** - gRPC support

**Rationale:** 
- Core stays small and fast (<5MB binary)
- Users only install what they need
- Packages can update independently
- Community can contribute packages
- Multiple implementations possible (aws-cli wrapper vs native SDK)

## Success Metrics

### 3 Months
- [ ] 100% Windows compatibility
- [ ] 10 real-world example scripts
- [ ] Package manager MVP
- [ ] 1000 GitHub stars

### 6 Months
- [ ] 50+ packages in ecosystem
- [ ] Used in production by 10+ companies
- [ ] Complete documentation
- [ ] 5000 GitHub stars

### 12 Months
- [ ] Replace bash/Python in major projects
- [ ] Conference talks about Wyn
- [ ] Corporate sponsors
- [ ] 10,000 GitHub stars

## Marketing Strategy

### Target Audience
1. **DevOps Engineers** - Tired of bash/Python mess
2. **SREs** - Need fast, reliable automation
3. **Platform Engineers** - Building internal tools
4. **Cloud Engineers** - Multi-cloud automation

### Key Messages
- "One language for all platforms"
- "Python simplicity, Go performance"
- "Stop context switching between languages"
- "Compiled scripts that just work"

### Launch Plan
1. **Blog post:** "Why I built Wyn"
2. **Show HN:** Demo with real examples
3. **Reddit:** r/devops, r/programming
4. **Twitter:** Thread with examples
5. **YouTube:** Tutorial series
6. **Conferences:** DevOps Days, KubeCon

## Open Questions

1. **Syntax:** Keep Python-style or evolve?
2. **Concurrency:** M:N threads or async/await?
3. **Memory:** GC-only or hybrid?
4. **Ecosystem:** Centralized packages or decentralized?
5. **Versioning:** Semantic versioning or date-based?

## Contributing

See [TODO.md](TODO.md) for current tasks.

## License

TBD - Likely MIT or Apache 2.0 for maximum adoption.

---

**Last Updated:** 2025-12-15
**Status:** Active Development
**Version:** 0.2.0
