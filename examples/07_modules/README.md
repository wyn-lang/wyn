# Module System Examples

Demonstrates Wyn v1.4.0's complete module system.

## Features Demonstrated

### 1. Nested Modules (`nested_example.wyn`)
- Java/TypeScript-style imports: `import utils.string`
- Short name resolution: `string::uppercase()`
- Filesystem organization: `utils/string.wyn`

### 2. Complete Example (`complete_example.wyn`)
- All module features in one file
- Nested modules
- Visibility control
- Relative imports (syntax shown)

### 3. Basic Modules (`main.wyn`, `math_user.wyn`)
- Simple module imports
- Function calls across modules

## Module System Features

### Nested Modules
```wyn
import network.http
var response = http::get("url");  // Short name
```

### Relative Imports
```wyn
import root::parent     // Parent module
import root::root       // Project root
import self::sibling     // Same directory
```

### Visibility Control
```wyn
pub fn public_api() {}   // Accessible from outside
fn private_helper() {}   // Private (default)
```

### Collision Detection
```wyn
import network.http
import storage.http  // Error: Ambiguous 'http'
```

## Running Examples

```bash
# Nested modules
./wyn examples/07_modules/nested_example.wyn
./examples/07_modules/nested_example.wyn.out

# Complete example
./wyn examples/07_modules/complete_example.wyn
./examples/07_modules/complete_example.wyn.out

# Basic modules
./wyn examples/07_modules/main.wyn
./examples/07_modules/main.wyn.out
```

## Module Structure

```
07_modules/
├── utils/
│   ├── string.wyn    # String utilities
│   └── math.wyn      # Math utilities
├── nested_example.wyn
├── complete_example.wyn
└── main.wyn
```

## Learn More

See [docs/language-guide.md](../../docs/language-guide.md#modules) for complete module documentation.
