# Wyn Language - Vision & Roadmap

**Last Updated:** December 17, 2025
**Progress:** 90% complete

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

### 1. ~~Type System is Too Weak~~ ✅ FIXED
**Status:** Generics partially implemented, Result/Option types working

### 2. ~~No Real Error Handling~~ ✅ FIXED
**Status:** Result<T, E> and Option<T> fully implemented with pattern matching

### 3. ~~String/Array Handling is Primitive~~ ✅ MOSTLY FIXED
**Status:** Array API implemented (len, get, push, set), slicing available

### 4. ~~No Package System~~ ✅ FIXED
**Status:** Package manager working, 3 official packages created

### 5. ~~Concurrency is Half-Baked~~ ✅ FIXED
**Status:** Spawn blocks fully functional with task_join_all synchronization

### 6. ~~Standard Library Gaps~~ ✅ FIXED
**Status:** SQLite, HTTP framework, testing framework all implemented

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

### ~~Month 1-2: Core Language~~ ✅ COMPLETE
- [x] Error handling (Result/Option types)
- [x] Pattern matching with destructuring
- [x] Concurrency (spawn blocks + task_join_all)
- [x] Standard library expansion
- [x] Package manager
- [x] 100% test pass rate

### Month 3: Package Ecosystem & Publishing
- [ ] Publish 3 official packages to GitHub
- [ ] Create package registry/discovery
- [ ] Test installation workflow
- [ ] Community package guidelines
- [ ] Package documentation

### Month 4: Cross-Platform & Community
- [ ] Windows testing and fixes
- [ ] Make repository public
- [ ] Launch blog post
- [ ] Show HN / Reddit posts
- [ ] Create Discord/forum
- [ ] CI/CD for all platforms

### Month 5: Advanced Features & Polish
- [ ] Try operator (?)
- [ ] Better generics
- [ ] Performance optimizations
- [ ] Debugger support
- [ ] Profiler

### Month 6: Ecosystem Growth
- [ ] 10+ community packages
- [ ] Production deployments
- [ ] Conference talks
- [ ] Tutorial series
- [ ] Best practices guide

## Killer Features for DevOps

### 1. Shebang Support ✅ DONE
### 2. Embedded Mode ✅ DONE
### 3. Enhanced REPL ✅ DONE
### 4. Script Caching ⚠️ TODO
### 5. Cross-Compilation ⚠️ TODO

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

### 3 Months ✅ MOSTLY ACHIEVED
- [x] 100+ real-world examples ✅
- [x] Package manager MVP ✅
- [x] 100% test pass rate ✅
- [x] 3 official packages ✅
- [ ] Windows compatibility (50% - needs testing)
- [ ] 1000 GitHub stars (pending public launch)

### 6 Months
- [ ] Packages published to GitHub
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
