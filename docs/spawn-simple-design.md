# Spawn System - Simple Design (Python-like)

## Philosophy

**One way to do things. Simple. Obvious.**

Like Python's threading/multiprocessing, but faster and safer.

## Core Concept: Just `spawn`

### Basic Spawn (What Works Today)

```wyn
spawn {
    print("Hello from thread")
}
```

### Spawn with Capture (v1.1)

```wyn
let x: int = 42
spawn {
    print(x)  // Just works - captured automatically
}
```

**That's it.** No special syntax, no handles, no complexity.

## Getting Results Back: Simple Queue

Instead of complex channels and handles, just use a shared list:

```wyn
let mut results: [int] = []

for i in 0..10 {
    spawn {
        let result: int = compute(i)
        results.append(result)  // Thread-safe append
    }
}

// Results automatically available after spawns complete
print(results)
```

**Python equivalent:**
```python
results = []
threads = []
for i in range(10):
    t = threading.Thread(target=lambda: results.append(compute(i)))
    t.start()
    threads.append(t)
for t in threads:
    t.join()
```

**Wyn is simpler** - no manual thread management!

## Shared State: Just Works

```wyn
let mut counter: int = 0

for i in 0..10 {
    spawn {
        counter = counter + 1  // Atomic by default
    }
}

print(counter)  // 10
```

**Python equivalent:**
```python
counter = 0
lock = threading.Lock()
threads = []
for i in range(10):
    def increment():
        global counter
        with lock:
            counter += 1
    t = threading.Thread(target=increment)
    t.start()
    threads.append(t)
for t in threads:
    t.join()
```

**Wyn is much simpler** - no locks, no context managers!

## Communication: Shared Lists

Instead of channels, just use lists:

```wyn
let mut queue: [int] = []

// Producer
spawn {
    for i in 0..100 {
        queue.append(i)
    }
}

// Consumer
spawn {
    while queue.len() > 0 {
        let item: int = queue.pop()
        process(item)
    }
}
```

**Simple, obvious, works.**

## Worker Pool: Just a Loop

```wyn
fn process_all(items: [str]) {
    for item in items {
        spawn {
            process(item)
        }
    }
    // Auto-waits at end of scope
}
```

**That's it.** No pool objects, no worker management.

## Implementation Requirements

### 1. Variable Capture (v1.1)
**What to implement:**
- Capture any variable from outer scope
- Automatic - no special syntax
- Works like Python closures

**Compiler:**
- Detect variables used in spawn block
- Generate capture struct
- Pass to thread function

**Runtime:**
- No changes needed (already have spawn)

### 2. Thread-Safe Collections (v1.1)
**What to implement:**
- `results.append(x)` - Atomic append
- `queue.pop()` - Atomic pop
- `counter = counter + 1` - Atomic increment

**Implementation:**
- Mutex per mutable variable
- Automatic locking on access
- No user-visible locks

### 3. Auto-Wait (Already Works!)
**Current behavior:**
```wyn
spawn { work() }
spawn { work() }
// Automatically waits here
```

**Keep this!** It's simpler than Go's WaitGroup.

## Comparison

### Go (Complex)
```go
var wg sync.WaitGroup
var mu sync.Mutex
results := []int{}

for i := 0; i < 10; i++ {
    wg.Add(1)
    go func(i int) {
        defer wg.Done()
        result := compute(i)
        mu.Lock()
        results = append(results, result)
        mu.Unlock()
    }(i)
}
wg.Wait()
```

### Wyn (Simple)
```wyn
let mut results: [int] = []

for i in 0..10 {
    spawn {
        results.append(compute(i))
    }
}
// Done!
```

**Wyn is 10x simpler.**

### Python (Verbose)
```python
import threading
results = []
lock = threading.Lock()
threads = []

for i in range(10):
    def worker(i):
        result = compute(i)
        with lock:
            results.append(result)
    t = threading.Thread(target=worker, args=(i,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()
```

### Wyn (Simple)
```wyn
let mut results: [int] = []

for i in 0..10 {
    spawn {
        results.append(compute(i))
    }
}
```

**Wyn is 5x simpler than Python.**

## What We DON'T Need

### ❌ Channels
Too complex. Just use lists.

### ❌ Explicit Handles
Auto-wait is simpler.

### ❌ WaitGroups
Auto-wait handles it.

### ❌ Explicit Locks
Automatic locking is safer.

### ❌ Worker Pools
Just spawn in a loop.

## What We DO Need

### ✅ Variable Capture
```wyn
let x: int = 42
spawn { print(x) }  // Just works
```

### ✅ Thread-Safe Append
```wyn
let mut results: [int] = []
spawn { results.append(42) }  // Atomic
```

### ✅ Atomic Operations
```wyn
let mut counter: int = 0
spawn { counter = counter + 1 }  // Atomic
```

**That's all we need.**

## Implementation Priority

### v1.1 (Next Release)
1. **Variable capture** - 3 days
2. **Thread-safe collections** - 2 days
3. **Atomic operations** - 1 day

**Total: 1 week of work**

### Result
After v1.1, Wyn will have:
- Simpler concurrency than Go
- Simpler concurrency than Python
- Faster than both
- Safer than both (automatic locking)

**One way to do things. Simple. Fast. Safe.**

## Example: Parallel Test Runner (v1.1)

```wyn
let mut passed: int = 0

for test in tests {
    spawn {
        if run_test(test) {
            passed = passed + 1  // Atomic increment
        }
    }
}

print(passed)  // Correct count, no race conditions
```

**Simple. Obvious. Fast.**

## Recommendation

**Implement v1.1 features only:**
- Variable capture
- Thread-safe collections
- Atomic operations

**Skip everything else.** Keep it simple.

**Wyn's concurrency will be simpler than Python, faster than Go.**
