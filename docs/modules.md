# Module System Guide

Complete guide to Wyn's module system (v1.4.0).

## Table of Contents
1. [Basic Modules](#basic-modules)
2. [Nested Modules](#nested-modules)
3. [Relative Imports](#relative-imports)
4. [Visibility Control](#visibility-control)
5. [Best Practices](#best-practices)

---

## Basic Modules

### Creating a Module

Create a file with public functions:

```wyn
// math_utils.wyn
pub fn add(a: int, b: int) -> int {
    return a + b;
}

pub fn multiply(a: int, b: int) -> int {
    return a * b;
}
```

### Importing and Using

```wyn
// main.wyn
import math_utils

fn main() -> int {
    var sum = math_utils::add(1, 2);
    var product = math_utils::multiply(3, 4);
    return sum + product;
}
```

---

## Nested Modules

Use `.` syntax like Java/TypeScript for nested module hierarchies.

### File Structure

```
project/
├── network/
│   ├── http.wyn
│   └── tcp.wyn
├── storage/
│   └── cache.wyn
└── main.wyn
```

### Importing Nested Modules

```wyn
// main.wyn
import network.http
import storage.cache

fn main() -> int {
    // Use short name (last part of path)
    var response = http::get("example.com");
    
    // Or use full path for disambiguation
    var data = network_http::post("api.com", "data");
    
    return 0;
}
```

### Short Name Resolution

When you import `network.http`, you can use:
- **Short name:** `http::function()` (recommended)
- **Full path:** `network_http::function()` (when needed)

---

## Relative Imports

Navigate module hierarchy without full paths.

### `root::` - Parent Module

```wyn
// In company/product/feature.wyn
import root::utils  // Resolves to: company/product/utils
```

### `root::` - Project Root

```wyn
// In any nested module
import root::config  // Always resolves to: config (at root)
```

### `self::` - Current Directory

```wyn
// In company/product/feature.wyn
import self::helpers  // Resolves to: company/product/feature/helpers
```

### Example Hierarchy

```
project/
├── config.wyn                    # Root config
├── database.wyn                  # Root database
└── company/
    ├── info.wyn                  # Company info
    └── product/
        ├── feature.wyn           # Feature module
        └── utils.wyn             # Feature utils
```

```wyn
// In company/product/feature.wyn
import root::utils           // company/product/utils
import root::root::info     // company/info
import root::config          // config (root)
import root::database        // database (root)
import self::helpers          // company/product/feature/helpers
```

---

## Visibility Control

Control what's accessible from outside your module.

### Public Functions

Use `pub` keyword to make functions accessible:

```wyn
// api.wyn
pub fn public_endpoint() -> int {
    return process_request();
}

fn process_request() -> int {  // Private by default
    return validate_input();
}

fn validate_input() -> int {   // Private by default
    return 42;
}
```

### Calling from Outside

```wyn
// main.wyn
import api

fn main() -> int {
    var result = api::public_endpoint();  // ✅ Works
    
    // var x = api::process_request();    // ❌ Error: Function is private
    // var y = api::validate_input();     // ❌ Error: Function is private
    
    return result;
}
```

### Error Messages

```
Error at line 10: Function 'process_request' in module 'api' is private
  Note: Only 'pub' functions can be called from outside the module
```

---

## Best Practices

### 1. Use Nested Modules for Organization

**Good:**
```
network/
  http.wyn
  tcp.wyn
  udp.wyn
storage/
  cache.wyn
  database.wyn
```

**Avoid:**
```
network_http.wyn
network_tcp.wyn
storage_cache.wyn
```

### 2. Use Short Names by Default

**Good:**
```wyn
import network.http
var x = http::get();  // Clean!
```

**Avoid (unless needed):**
```wyn
import network.http
var x = network_http::get();  // Verbose
```

### 3. Use Full Paths for Disambiguation

When you have collisions:

```wyn
import network.http
import storage.http  // Error: Ambiguous 'http'

// Fix: Use full paths
var x = network_http::get();
var y = storage_http::get();
```

### 4. Use Relative Imports in Large Projects

**Good:**
```wyn
// In company/product/feature.wyn
import root::utils
import root::config
```

**Avoid:**
```wyn
// Brittle to refactoring
import company.product.utils
import config
```

### 5. Keep Public API Minimal

```wyn
// Only expose what users need
pub fn create_user() {}
pub fn delete_user() {}

// Keep implementation private
fn validate_email() {}
fn hash_password() {}
fn send_notification() {}
```

### 6. One Module Per File

Each `.wyn` file is one module. Keep modules focused and cohesive.

---

## Advanced Features

### Module Name Collision Detection

Wyn automatically detects when two modules have the same short name:

```wyn
import network.http
import storage.http

// Error: Ambiguous module name 'http'
//   Could refer to:
//     - network/http (imported at line 1)
//     - storage/http (imported at line 2)
//   Use full path to disambiguate:
//     - network_http::function()
//     - storage_http::function()
```

### Filesystem Organization

- Module paths use `/` on filesystem
- Import paths use `.` in code
- Example: `import network.http` → `network/http.wyn`

---

## Examples

See `examples/07_modules/` for working examples:
- `nested_example.wyn` - Nested modules with short names
- `complete_example.wyn` - All features demonstrated
- `main.wyn` - Basic module usage

---

## Summary

Wyn's module system provides:
- ✅ Clean syntax (Java/TypeScript style)
- ✅ Safety (collision detection, visibility)
- ✅ Convenience (short names, relative imports)
- ✅ Performance (zero overhead, compile-time)

**Production-ready for projects of any size!**
