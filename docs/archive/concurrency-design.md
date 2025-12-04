# Wyn Concurrency - Simple Design

## Problem with Go's Model
- WaitGroups, Mutexes, Channels = complex
- Too many primitives to learn
- Easy to get wrong (deadlocks, race conditions)

## Wyn's Simple Model

### Concept: Automatic Synchronization
```wyn
# Spawn returns a Future that auto-waits
let result = spawn { compute(42) }
print(result)  # Automatically waits here
```

### No Manual Synchronization Needed
```wyn
# Multiple spawns
let a = spawn { work(1) }
let b = spawn { work(2) }
let c = spawn { work(3) }

# Auto-wait when accessed
print(a + b + c)  # Waits for all three
```

### Shared State: Atomic by Default
```wyn
let mut counter = 0

spawn { counter = counter + 1 }  # Atomic increment
spawn { counter = counter + 1 }  # Atomic increment

# Auto-wait and print
print(counter)  # Always correct (2)
```

## Implementation

### 1. spawn Returns Future
```c
// In codegen
case STMT_SPAWN:
    // Create thread
    // Store future handle
    // Return immediately
```

### 2. Future Auto-Waits on Access
```c
// When variable is used
if (is_future(var)) {
    // Wait for completion
    // Get result
}
```

### 3. Atomic Operations
```c
// All shared variable access is atomic
// Use CPU atomic instructions
// No manual locks needed
```

## Advantages

### vs Go
- ✅ **Simpler** - No WaitGroups, Mutexes, Channels
- ✅ **Safer** - No deadlocks, automatic sync
- ✅ **Easier** - Just use the value, it waits

### vs Async/Await
- ✅ **No keywords** - spawn is enough
- ✅ **No coloring** - All functions work
- ✅ **Implicit** - Waits automatically

## Examples

### Parallel Map
```wyn
fn parallel_map(arr: [int], f: fn(int) -> int) -> [int] {
    let mut results = []
    
    for item in arr {
        let future = spawn { f(item) }
        results.push(future)
    }
    
    return results  # Auto-waits for all
}
```

### Parallel Sum
```wyn
fn parallel_sum(arr: [int]) -> int {
    let mid = arr.len() / 2
    let left = spawn { sum(arr[0:mid]) }
    let right = spawn { sum(arr[mid:]) }
    
    return left + right  # Auto-waits
}
```

### Web Server
```wyn
fn handle_request(req: Request) -> Response {
    # Each request auto-spawned
    let data = spawn { fetch_data() }
    let html = spawn { render(data) }
    return html  # Auto-waits
}
```

## Implementation Plan

### Phase 1: Basic spawn (Week 1)
- [ ] spawn creates OS thread
- [ ] Returns Future handle
- [ ] Auto-wait on variable access
- [ ] Test with simple examples

### Phase 2: Atomic Operations (Week 2)
- [ ] Make all shared access atomic
- [ ] Use CPU atomic instructions
- [ ] Test race conditions

### Phase 3: Optimization (Week 3)
- [ ] Thread pool (reuse threads)
- [ ] Work stealing
- [ ] Reduce overhead

## Performance Target

```
Sequential: 1.09s
Go:         0.01s (109x speedup)
Wyn Target: 0.05s (20x speedup)
```

**Gap**: 5x slower than Go (acceptable for v1.0)

## Syntax

### Current (Broken)
```wyn
spawn {
    work()
}
# No way to wait ❌
```

### Proposed (Simple)
```wyn
let result = spawn { work() }
print(result)  # Auto-waits ✅
```

## Benefits

1. **No new keywords** - Just spawn
2. **No manual sync** - Automatic
3. **No deadlocks** - Can't happen
4. **No race conditions** - Atomic by default
5. **Easy to use** - Like sequential code

## Trade-offs

### Pros
- Much simpler than Go
- Safer (no deadlocks)
- Easier to learn

### Cons
- Less control than Go
- May be slower (auto-wait overhead)
- Can't do complex patterns

## Decision

**Use automatic synchronization** - simpler, safer, easier.

If users need Go-level control, they can use FFI to call Go code.
