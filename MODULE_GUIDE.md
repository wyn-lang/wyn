# Wyn Module System Guide

Complete guide to using the Wyn module system.

## Overview

The module system allows you to organize code across multiple files and create reusable libraries.

## Basic Concepts

### Modules
A module is a Wyn file that exports functions, structs, or other items.

### Exports
Use `export` to make items public.

### Imports
Use `import` to use items from other modules.

### Namespaces
Access imported items with `::` syntax.

## Quick Start

### Step 1: Create a Module

**math.wyn:**
```wyn
export fn add(a: int, b: int) -> int {
    return a + b;
}

export fn multiply(a: int, b: int) -> int {
    return a * b;
}
```

### Step 2: Import and Use

**main.wyn:**
```wyn
import math;

fn main() -> int {
    let sum = math::add(10, 20);
    let product = math::multiply(5, 6);
    return sum + product;
}
```

### Step 3: Build

```bash
./wyn build myproject/
./myproject/main
```

## Exporting

### Export Functions
```wyn
export fn public_function() -> int {
    return 42;
}

fn private_function() -> int {
    return 0;  // Not exported
}
```

### Export Structs
```wyn
export struct Point {
    x: int,
    y: int
}
```

### Multiple Exports
```wyn
export fn func1() -> int { return 1; }
export fn func2() -> int { return 2; }
export fn func3() -> int { return 3; }
```

## Importing

### Single Import
```wyn
import math;

fn main() -> int {
    return math::add(1, 2);
}
```

### Multiple Imports
```wyn
import math;
import utils;
import helpers;

fn main() -> int {
    let a = math::add(1, 2);
    let b = utils::process(a);
    return helpers::finalize(b);
}
```

## Namespace Resolution

### Using Imported Functions
```wyn
import math;

fn main() -> int {
    return math::add(1, 2);  // module::function
}
```

### Avoiding Conflicts
```wyn
import math;

// Local function doesn't conflict
fn add(a: int, b: int) -> int {
    return a + b + 1;
}

fn main() -> int {
    let local_sum = add(1, 2);        // Calls local function
    let module_sum = math::add(1, 2); // Calls module function
    return local_sum + module_sum;
}
```

## Module Organization

### Flat Structure
```
project/
  main.wyn
  math.wyn
  utils.wyn
```

### Nested Structure
```
project/
  main.wyn
  lib/
    math.wyn
    utils.wyn
```

**Note:** Currently, all `.wyn` files in the project directory are included.

## Best Practices

### 1. One Module Per File
Keep each module in its own file.

**Good:**
```
math.wyn      # Math functions
string.wyn    # String functions
io.wyn        # I/O functions
```

### 2. Export Only Public API
Don't export internal helper functions.

```wyn
// Public API
export fn calculate(x: int) -> int {
    return internal_helper(x) * 2;
}

// Private helper
fn internal_helper(x: int) -> int {
    return x + 1;
}
```

### 3. Use Descriptive Module Names
```wyn
import math;        // Good
import m;           // Bad
```

### 4. Group Related Functions
```wyn
// math.wyn - All math operations
export fn add(a: int, b: int) -> int { ... }
export fn subtract(a: int, b: int) -> int { ... }
export fn multiply(a: int, b: int) -> int { ... }
```

## Common Patterns

### Utility Module
```wyn
// utils.wyn
export fn is_even(n: int) -> bool {
    return n % 2 == 0;
}

export fn is_odd(n: int) -> bool {
    return n % 2 != 0;
}
```

### Data Module
```wyn
// data.wyn
export struct Config {
    host: string,
    port: int
}

export fn default_config() -> Config {
    return Config { host: "localhost", port: 8080 };
}
```

### Service Module
```wyn
// service.wyn
export fn start_service() -> int {
    return initialize() + configure();
}

fn initialize() -> int {
    return 1;  // Private
}

fn configure() -> int {
    return 2;  // Private
}
```

## Examples

### Example 1: Math Library

**math.wyn:**
```wyn
export fn add(a: int, b: int) -> int {
    return a + b;
}

export fn subtract(a: int, b: int) -> int {
    return a - b;
}

export fn multiply(a: int, b: int) -> int {
    return a * b;
}
```

**main.wyn:**
```wyn
import math;

fn main() -> int {
    let sum = math::add(10, 5);
    let diff = math::subtract(10, 5);
    let prod = math::multiply(10, 5);
    
    print(sum);   // 15
    print(diff);  // 5
    print(prod);  // 50
    
    return 0;
}
```

### Example 2: Multiple Modules

**math.wyn:**
```wyn
export fn square(x: int) -> int {
    return x * x;
}
```

**utils.wyn:**
```wyn
export fn double(x: int) -> int {
    return x + x;
}
```

**main.wyn:**
```wyn
import math;
import utils;

fn main() -> int {
    let x = 5;
    let squared = math::square(x);
    let doubled = utils::double(x);
    return squared + doubled;  // 25 + 10 = 35
}
```

## Limitations

### Current Limitations
1. No nested module paths (e.g., `math::advanced::func`)
2. No selective imports (e.g., `import math::{add, subtract}`)
3. No module aliases (e.g., `import math as m`)
4. All files in directory are included

### Future Features
- Nested module hierarchies
- Selective imports
- Module aliases
- Package management

## Troubleshooting

### "Undefined variable 'module::function'"
- Ensure module file exists
- Check function is exported
- Verify import statement

### "Redefinition of 'function'"
- Check for name conflicts with built-in functions
- Rename your function or use different module name

### "No .wyn files found"
- Ensure all module files are in project directory
- Check file extensions are `.wyn`

## See Also

- [Language Guide](LANGUAGE_GUIDE.md)
- [Build Guide](BUILD_GUIDE.md)
- [Standard Library](STDLIB_REFERENCE.md)
