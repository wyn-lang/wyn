# Extension Methods in Wyn

## Overview

Wyn supports Python/JavaScript style extension methods! Add custom methods to any type using clean, intuitive syntax.

## Syntax

```wyn
fn TypeName.method_name(self, args...) -> ReturnType {
    // method body
}
```

## ✅ Fully Working: Int Extensions

```wyn
fn int.squared(self) -> int {
    return self * self
}

fn int.cubed(self) -> int {
    return self * self * self
}

fn int.is_even(self) -> int {
    return self % 2 == 0 ? 1 : 0
}

fn main() {
    print(5.squared())              // 25 ✅
    print(3.cubed())                // 27 ✅
    
    const num = 7
    print(num.squared())            // 49 ✅
    
    // METHOD CHAINING WORKS!
    print((-10).abs().squared())    // 100 ✅
}
```

## ⚠️ Partial: String Extensions

**What works:**
```wyn
fn str.echo(self) -> str {
    return self  // ✅ Works!
}

"hello".echo()  // "hello"
```

**What doesn't work yet:**
```wyn
fn str.shout(self) -> str {
    return self + "!!!"  // ❌ Crashes
}

fn str.loud(self) -> str {
    return self.upper()  // ❌ Crashes
}
```

**Issue**: Using `self` in expressions or calling methods on `self` crashes during compilation. This will be fixed in v0.4.0.

## Current Status (v0.3.0)

### ✅ Fully Working
- **Int extensions** - Complete with all operations
- **Method chaining** - Works perfectly
- **Literals and variables** - Both work

### ⚠️ Partial Support
- **String extensions** - Can return self, but can't use in expressions
- **Array extensions** - Not yet tested

## Examples

### Int Extensions (Working!)
```wyn
fn int.squared(self) -> int {
    return self * self
}

fn int.double(self) -> int {
    return self * 2
}

fn int.is_positive(self) -> int {
    return self > 0 ? 1 : 0
}

// Use them:
print(5.squared())              // 25
print(21.double())              // 42
print((-5).is_positive())       // 0

// Method chaining:
print((-10).abs().squared())    // 100
```

## Comparison

**Python:**
```python
class MyInt(int):
    def squared(self):
        return self * self

MyInt(5).squared()  # 25
```

**JavaScript:**
```javascript
Number.prototype.squared = function() {
    return this * this
}

(5).squared()  // 25
```

**Wyn:**
```wyn
fn int.squared(self) -> int {
    return self * self
}

5.squared()  // 25
```

## Benefits

✨ **Clean Syntax** - Just like Python/JavaScript  
✨ **Type Safe** - Compile-time checking  
✨ **Method Chaining** - Compose operations elegantly  
✨ **No Wrappers** - Extend types directly  
✨ **Powerful** - Add any functionality you need  

## Summary

Extension methods are **fully working for int type** and provide exactly the Python/JavaScript style API!

```wyn
fn int.squared(self) -> int { return self * self }

5.squared()  // 25 - IT WORKS! 🎉
```

String extensions work for simple cases (returning self) but need fixes for expressions. This will be completed in v0.4.0.

**Int extensions are production-ready and incredibly powerful!**
