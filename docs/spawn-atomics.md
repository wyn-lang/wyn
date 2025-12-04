# Spawn Atomic Operations

## Overview

Wyn v1.0.0 implements **automatic atomic operations** for all mutable variables captured in `spawn` blocks. This prevents race conditions and ensures thread-safe concurrent execution.

## How It Works

When a mutable variable is captured in a `spawn` block, the compiler automatically:

1. **Marks the variable as shared** - Tracks which variables need atomic operations
2. **Uses atomic instructions** - All reads and writes use atomic CPU instructions
3. **Implements atomic RMW** - Read-modify-write operations (like `+=`) are fully atomic

## Atomic Operations

### Simple Assignment (x86_64)
```asm
# counter = 42
lock xchgq %rax, -8(%rbp)
```

### Compound Assignment (x86_64)
```asm
# counter = counter + 1
lock addq %rcx, -8(%rbp)
```

### Atomic RMW with CAS (ARM64)
```asm
# counter = counter + 1
L_cas_1:
    ldxr x0, [x19]      # Load exclusive
    add x2, x0, x1      # Compute new value
    stxr w3, x2, [x19]  # Store exclusive
    cbnz w3, L_cas_1    # Retry if failed
```

## Examples

### Counter Increment
```wyn
fn main() {
    let mut counter: int = 0
    
    spawn { counter = counter + 1 }
    spawn { counter = counter + 1 }
    spawn { counter = counter + 1 }
    
    sleep_ms(100)
    assert(counter == 3)  // Always exactly 3, no race conditions
}
```

### Multiple Variables
```wyn
fn main() {
    let mut x: int = 0
    let mut y: int = 0
    
    spawn {
        x = x + 10
        y = y + 20
    }
    
    spawn {
        x = x + 5
        y = y + 15
    }
    
    sleep_ms(100)
    assert(x == 15)  // Always correct
    assert(y == 35)  // Always correct
}
```

### Immutable Captures (No Atomics)
```wyn
fn main() {
    let x: int = 42  // Immutable
    let mut result: int = 0
    
    spawn {
        result = x  // Only 'result' uses atomics, 'x' is read-only
    }
    
    sleep_ms(100)
    assert(result == 42)
}
```

## Supported Operations

### Atomic Operations
- ✅ `counter = value` - Atomic store
- ✅ `counter = counter + 1` - Atomic add
- ✅ `counter = counter - 1` - Atomic subtract
- ✅ `counter = counter * 2` - Atomic multiply (CAS loop)
- ✅ `counter = counter / 2` - Atomic divide (CAS loop)
- ✅ `counter = counter % 2` - Atomic modulo (CAS loop)

### Implementation Details

**x86_64:**
- Uses `lock` prefix for atomic operations
- `lock addq` / `lock subq` for `+=` / `-=`
- `lock cmpxchgq` for multiply/divide/modulo (CAS loop)
- `lock xchgq` for simple stores

**ARM64:**
- Uses `ldxr` / `stxr` (load/store exclusive) for CAS loops
- Uses `ldar` / `stlr` (load/store acquire/release) for simple operations
- Automatic retry on contention

## Performance

- **Zero overhead for non-shared variables** - Only captured mutable variables use atomics
- **Optimized for common cases** - `+=` and `-=` use single atomic instructions
- **Lock-free** - All operations are lock-free using CPU atomic instructions
- **Tested under load** - 50+ concurrent spawns with zero race conditions

## Testing

All spawn tests verify atomic behavior:

```bash
# Run spawn tests
./build/stage0 -o test tests/spawn_shared_counter_test.wyn
./test  # Always passes, counter is exactly 10

./build/stage0 -o test tests/spawn_race_condition_test.wyn
./test  # Always passes, counter is exactly 50
```

## Limitations

### Current Limitations
- ⚠️ **Spawn in loops not yet supported** - Use explicit spawn statements
- ⚠️ **Arrays/structs** - Only primitive types (int, float, str) are atomic

### Workarounds

Instead of:
```wyn
for i in 0..100 {
    spawn { counter = counter + 1 }  // Not yet supported
}
```

Use:
```wyn
spawn { counter = counter + 1 }
spawn { counter = counter + 1 }
// ... repeat 100 times
```

Or use a helper function:
```wyn
fn spawn_workers(count: int) {
    // Implementation pending
}
```

## Future Enhancements

- [ ] Atomic operations for arrays (atomic append)
- [ ] Atomic operations for structs (atomic field updates)
- [ ] Spawn in for loops
- [ ] Atomic compare-and-swap primitives exposed to user code
- [ ] Memory ordering controls (relaxed, acquire, release, seq_cst)

## Conclusion

Wyn's automatic atomic operations make concurrent programming **safe by default**. You don't need to think about locks, mutexes, or atomic primitives - the compiler handles it all automatically.

**No race conditions. No data races. Just correct concurrent code.**
