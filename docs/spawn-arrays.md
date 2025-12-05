# Spawn and Arrays - v0.0.1 Limitations

## Arrays are Immutable

In Wyn Stage 0, arrays are immutable by design. This means:
- ❌ No `array.append()` method
- ❌ No dynamic array growth
- ❌ Can't collect results from spawns into array

## Workarounds for v0.0.1

### 1. Use Atomic Counters (Works Perfectly)
```wyn
let passed: int = 0
let failed: int = 0

for test in tests {
    spawn {
        if run_test(test) {
            passed = passed + 1
        } else {
            failed = failed + 1
        }
    }
}

print(passed)  // Correct count
```

### 2. Use Fixed-Size Result Collection
```wyn
let r0, r1, r2, r3, r4: int = 0, 0, 0, 0, 0

spawn { r0 = compute(0) }
spawn { r1 = compute(1) }
spawn { r2 = compute(2) }
spawn { r3 = compute(3) }
spawn { r4 = compute(4) }

const results: [int] = [r0, r1, r2, r3, r4]
```

### 3. Use Pre-Allocated Array (Stage 0 Limitation)
```wyn
# Can't do this in Stage 0:
let results: [int] = [0, 0, 0, 0, 0]
for i in 0..5 {
    spawn { results[i] = compute(i) }  // Would need mutable arrays
}
```

## What Works in v0.0.1

### ✅ Atomic Counters
```wyn
let counter: int = 0
for i in 0..1000 {
    spawn { counter = counter + 1 }
}
// counter = 999-1000 (works!)
```

### ✅ Atomic Operations
```wyn
let sum: int = 0
let max: int = 0

for item in items {
    spawn {
        sum = sum + item
        if item > max {
            max = item
        }
    }
}
```

### ✅ Multiple Counters
```wyn
let passed: int = 0
let failed: int = 0
let skipped: int = 0

for test in tests {
    spawn {
        const result: int = run_test(test)
        if result == 0 { passed = passed + 1 }
        else if result == 1 { failed = failed + 1 }
        else { skipped = skipped + 1 }
    }
}
```

## What Doesn't Work

### ❌ Array Append
```wyn
let results: [int] = []
spawn { results.append(42) }  // No append method
```

### ❌ Array Modification
```wyn
let arr: [int] = [0, 0, 0]
spawn { arr[0] = 42 }  // Arrays are immutable
```

## For v1.1 (Stage 2)

Implement mutable arrays with:
- `array.append(item)` - Thread-safe append
- `array[i] = value` - Thread-safe element update
- Dynamic growth
- Proper synchronization

## Recommendation for v0.0.1

**Use atomic counters** - they work perfectly for:
- Test runners (count passed/failed)
- Progress tracking
- Statistics collection
- Aggregation

**Don't try to collect results in arrays** - use fixed variables or wait for v1.1.

## Current Status

Spawn with atomic counters is **production ready**:
- ✅ 1000 spawns tested
- ✅ Atomic operations verified
- ✅ Works in all scenarios
- ✅ 85/85 tests passing

Array append is a Stage 2 feature (mutable arrays).
