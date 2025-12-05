# spawn Implementation - Simple Model

## Current State
- Parser: ✅ Parses spawn blocks
- Type checker: ✅ Type checks spawn
- Codegen: ❌ **NOT IMPLEMENTED** (causes hang)

## Simple Design

### No Complex Primitives
Instead of Go's WaitGroups/Mutexes/Channels, use:

**Automatic join on scope exit**

```wyn
fn main() {
    spawn { work(1) }
    spawn { work(2) }
    spawn { work(3) }
}  # Auto-waits here before exit
```

### Implementation

#### 1. Track Spawned Threads
```c
typedef struct {
    pthread_t threads[1024];
    int count;
} SpawnTracker;
```

#### 2. spawn Creates Thread
```c
case STMT_SPAWN:
    // Generate thread function
    // pthread_create()
    // Store handle in tracker
```

#### 3. Auto-Join on Scope Exit
```c
// At end of function/block
for (int i = 0; i < tracker.count; i++) {
    pthread_join(tracker.threads[i], NULL);
}
```

## Syntax

### Basic spawn
```wyn
fn main() {
    spawn { print_str("Hello") }
    spawn { print_str("World") }
}  # Waits for both before exit
```

### With shared state (atomic)
```wyn
fn main() {
    let counter = 0
    
    spawn { counter = counter + 1 }
    spawn { counter = counter + 1 }
    
    print(counter)  # Always 2 (atomic ops)
}
```

## Implementation Steps

1. Add SpawnTracker to CodeGen
2. Generate thread wrapper function
3. Call pthread_create in STMT_SPAWN
4. Add auto-join at scope exit
5. Make shared variables atomic

## Advantages

- ✅ No manual synchronization
- ✅ No deadlocks possible
- ✅ Simple to use
- ✅ Safe by default

## Limitations

- Can't spawn and forget (always waits)
- Less control than Go
- May be slower (auto-join overhead)

**Trade-off**: Simplicity over control
