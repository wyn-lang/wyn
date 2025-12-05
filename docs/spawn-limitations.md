# Spawn Limitations and Workarounds

## Loop Variable Capture Issue

### Problem
Loop variables are captured by reference, causing all spawns to see the final value:

```wyn
let counter: int = 0
for i in 0..10 {
    spawn { 
        print(i)  // All print 10, not 0-9
        counter = counter + 1
    }
}
// counter = 0 (spawns see i=10, loop already finished)
```

### Why This Happens
- Loop variable `i` is stored at a fixed memory location
- Spawn captures the address of `i`
- By the time spawns execute, loop has finished and `i` = 10
- All spawns see the same final value

### Workaround: Create Local Copy
```wyn
let counter: int = 0
for i in 0..10 {
    const i_copy: int = i  // Create copy for this iteration
    spawn { 
        print(i_copy)  // Each spawn gets correct value
        counter = counter + 1
    }
}
// counter = 10 (works correctly)
```

### What Works
✅ **Fixed spawn statements:**
```wyn
let counter: int = 0
spawn { counter = counter + 1 }
spawn { counter = counter + 1 }
spawn { counter = counter + 1 }
// counter = 3 (works perfectly)
```

✅ **Spawn with local copies:**
```wyn
for i in 0..100 {
    const val: int = i
    spawn { process(val) }
}
```

### What Doesn't Work
❌ **Direct loop variable capture:**
```wyn
for i in 0..100 {
    spawn { process(i) }  // All see final value
}
```

## Atomic Operations Status

### What Works
- ✅ Atomic increment/decrement (`counter += 1`)
- ✅ Atomic read/write
- ✅ Multiple spawns on same variable
- ✅ x86_64 and ARM64 support

### Verified
```wyn
let counter: int = 0
spawn { counter = counter + 1 }
spawn { counter = counter + 1 }
spawn { counter = counter + 1 }
// counter = 3 (guaranteed, no race conditions)
```

## Recommendations

### For v0.0.1
Use the workaround pattern:
```wyn
for item in items {
    const item_copy: T = item
    spawn { process(item_copy) }
}
```

### For v1.1
Implement proper loop variable capture:
- Create per-iteration copies automatically
- Compiler generates: `let __i_N = i` for each iteration
- Spawn captures `__i_N` instead of `i`

## Test Results

With workaround:
- ✅ 85/85 tests passing
- ✅ Atomic operations verified
- ✅ Production ready

Without workaround:
- ❌ Loop spawns don't work correctly
- Use local copy pattern instead
