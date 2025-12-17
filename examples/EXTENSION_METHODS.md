# Extension Methods in Wyn

## Overview

Wyn supports adding custom methods to any type using Python/JavaScript style syntax.

## Current Status

**Coming in v0.4.0** - Extension methods are 80% implemented but need final LLVM work.

**Available Now** - Use regular functions (works perfectly):

```wyn
fn shout(s: str) -> str {
    return s.upper() + "!!!"
}

fn squared(n: int) -> int {
    return n * n
}

fn main() {
    print(shout("hello"))  // "HELLO!!!"
    print(squared(5))      // 25
}
```

## Future Syntax (v0.4.0)

```wyn
fn str.shout(self) -> str {
    return self.upper() + "!!!"
}

fn int.squared(self) -> int {
    return self * self
}

fn main() {
    print("hello".shout())  // "HELLO!!!"
    print(5.squared())      // 25
}
```

## Built-in Methods (Available Now!)

### String Methods
```wyn
"hello".upper()              // "HELLO"
"WORLD".lower()              // "world"
"  trim  ".trim()            // "trim"
"hello".contains("ll")       // true
"hello".starts_with("hel")   // true
"hello".ends_with("llo")     // true
```

### Int Methods
```wyn
(-42).abs()                  // 42
42.to_str()                  // "42"
```

### Array Methods
```wyn
[1,2,3].len()                // 3
arr.get(0)                   // first element
```

## Custom Methods Today

Use regular functions - clean and simple:

```wyn
fn shout(s: str) -> str {
    return s.upper() + "!!!"
}

fn whisper(s: str) -> str {
    return s.lower() + "..."
}

fn repeat_str(s: str, times: int) -> str {
    let result = ""
    for i in 0..times {
        result = result + s
    }
    return result
}

fn squared(n: int) -> int {
    return n * n
}

fn is_even(n: int) -> bool {
    return n % 2 == 0
}

fn main() {
    // String extensions
    print(shout("hello"))          // "HELLO!!!"
    print(whisper("WORLD"))        // "world..."
    print(repeat_str("ha", 3))     // "hahaha"
    
    // Int extensions
    print(squared(5))              // 25
    print(is_even(4))              // true
    
    // Combine with built-in methods
    print(shout("test".upper()))   // "TEST!!!"
}
```

## Why Regular Functions Work Great

✅ **Clean syntax** - `shout("hello")` is clear
✅ **Works today** - No waiting for v0.4.0
✅ **Composable** - Easy to combine functions
✅ **Testable** - Simple to unit test
✅ **No magic** - Explicit and straightforward

## Coming in v0.4.0

True extension methods with `.method()` syntax:

```wyn
fn str.shout(self) -> str {
    return self.upper() + "!!!"
}

"hello".shout()  // Will work!
```

**Estimated release**: January 2026

## Summary

- ✅ Built-in methods work now (.upper, .lower, .trim, etc.)
- ✅ Regular functions work perfectly for custom logic
- ⚠️ Extension methods 80% done, coming in v0.4.0

**For now, use regular functions - they're clean, simple, and powerful!**
