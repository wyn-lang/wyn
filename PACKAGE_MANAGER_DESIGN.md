# Wyn Package Manager Design

## Overview

The Wyn package manager enables code sharing and ecosystem growth while keeping the core language lean.

## Goals

1. **Simple** - Easy to publish and install packages
2. **Decentralized** - Use GitHub/Git as registry
3. **Fast** - Minimal overhead
4. **Secure** - Verify checksums and signatures
5. **Cross-platform** - Works on Linux, macOS, Windows

## Package Format

### package.wyn
```wyn
package {
    name: "http-router"
    version: "1.0.0"
    author: "username"
    description: "HTTP routing library"
    license: "MIT"
    
    dependencies: [
        "github.com/user/json-parser@1.2.0"
    ]
    
    files: [
        "src/router.wyn"
        "src/middleware.wyn"
    ]
}
```

### Directory Structure
```
my-package/
├── package.wyn          # Package manifest
├── README.md            # Documentation
├── src/                 # Source files
│   ├── main.wyn
│   └── utils.wyn
├── tests/               # Tests
│   └── test_main.wyn
└── examples/            # Examples
    └── demo.wyn
```

## Commands

### Install Package
```bash
wyn pkg install github.com/user/package
wyn pkg install github.com/user/package@1.2.0
wyn pkg install ./local/path
```

### Publish Package
```bash
wyn pkg publish
```

### List Packages
```bash
wyn pkg list              # List installed
wyn pkg search http       # Search registry
```

### Update Packages
```bash
wyn pkg update            # Update all
wyn pkg update package    # Update specific
```

### Remove Package
```bash
wyn pkg remove package
```

## Package Registry

### Option 1: GitHub-based (Recommended)
- Packages are GitHub repos
- Use GitHub releases for versions
- No central server needed
- Use GitHub API for search

**Format:** `github.com/user/repo`

### Option 2: Central Registry
- Similar to npm, crates.io
- Requires hosting
- More control
- Better search

**Start with Option 1, migrate to Option 2 later**

## Installation Location

### User Packages
```
~/.wyn/packages/
├── github.com/
│   └── user/
│       └── package/
│           ├── package.wyn
│           └── src/
```

### Project Packages
```
project/
├── wyn_modules/         # Like node_modules
│   └── package/
└── wyn.lock             # Lock file
```

## Import Resolution

### Before Package Manager
```wyn
import io    # From stdlib only
```

### After Package Manager
```wyn
import io                              # From stdlib
import github.com/user/http-router     # From package
import ./local/module                  # Local file
```

## Implementation Plan

### Phase 1: Basic Install (Week 1)
- [ ] Parse package.wyn format
- [ ] `wyn pkg install github.com/user/repo`
- [ ] Download from GitHub
- [ ] Extract to ~/.wyn/packages/
- [ ] Update import resolution

### Phase 2: Dependencies (Week 2)
- [ ] Parse dependencies from package.wyn
- [ ] Recursive dependency installation
- [ ] Version resolution
- [ ] Create wyn.lock file

### Phase 3: Publish (Week 3)
- [ ] `wyn pkg publish` command
- [ ] Create GitHub release
- [ ] Generate checksums
- [ ] Tag version

### Phase 4: Advanced (Week 4)
- [ ] `wyn pkg search`
- [ ] `wyn pkg update`
- [ ] Checksum verification
- [ ] Signature verification

## Package Examples

### Example 1: HTTP Router
```
github.com/wyn-lang/http-router

package.wyn:
  name: "http-router"
  version: "1.0.0"
  
src/router.wyn:
  pub fn create_router() -> Router { ... }
  pub fn add_route(r: Router, path: str, handler: fn) { ... }
```

**Usage:**
```wyn
import github.com/wyn-lang/http-router as router

fn main() {
    const r = router.create_router()
    router.add_route(r, "/", handler_home)
}
```

### Example 2: AWS SDK
```
github.com/wyn-lang/aws

package.wyn:
  name: "aws"
  version: "2.0.0"
  dependencies: ["github.com/wyn-lang/http-client@1.0.0"]
  
src/s3.wyn:
  pub fn upload(bucket: str, key: str, data: str) -> bool { ... }
```

**Usage:**
```wyn
import github.com/wyn-lang/aws as aws

fn main() {
    aws.s3_upload("my-bucket", "file.txt", "content")
}
```

## Security

### Checksums
- Generate SHA256 for each file
- Store in package.wyn
- Verify on install

### Signatures (Future)
- Sign packages with GPG
- Verify signatures
- Trust model

## Compatibility

### Versioning
- Semantic versioning: MAJOR.MINOR.PATCH
- Breaking changes: increment MAJOR
- New features: increment MINOR
- Bug fixes: increment PATCH

### Version Resolution
- Use latest compatible version
- Lock file prevents surprises
- Can specify exact version: `@1.2.3`

## CLI Implementation

### wyn pkg install
```c
// In src/wyn.c
int run_pkg_install(const char* package) {
    // Parse package URL
    // Clone from GitHub
    // Extract to ~/.wyn/packages/
    // Update import paths
}
```

### wyn pkg publish
```c
int run_pkg_publish() {
    // Read package.wyn
    // Create GitHub release
    // Tag version
    // Generate checksums
}
```

## Benefits

1. **Ecosystem growth** - Easy to share code
2. **Code reuse** - Don't reinvent wheel
3. **Specialization** - AWS, databases, etc. as packages
4. **Community** - Contributors can publish packages
5. **Lean core** - Core stays small and fast

## Timeline

- **Week 1:** Basic install working
- **Week 2:** Dependencies working
- **Week 3:** Publish working
- **Month 2:** 10+ packages available
- **Month 3:** 50+ packages available

## First Packages to Create

1. **aws** - AWS SDK
2. **postgres** - PostgreSQL driver
3. **http-router** - HTTP routing framework
4. **testing** - Testing framework
5. **cli-framework** - CLI app framework
6. **docker** - Docker API client
7. **k8s** - Kubernetes client
8. **redis** - Redis client

## Success Metrics

- **Month 1:** Package manager working, 5 packages
- **Month 3:** 50 packages, 100 users
- **Month 6:** 200 packages, 1000 users

## Conclusion

Package manager is critical for Wyn's success. Enables ecosystem growth while keeping core lean. GitHub-based approach is simple and requires no infrastructure.

**Next Step:** Implement Phase 1 (basic install)
