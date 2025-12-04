# Spawn System Design - Full Concurrency

## Current Limitations

### What Works
- ✅ `spawn { independent_function() }` - Independent tasks
- ✅ Auto-join at scope exit
- ✅ Direct pthread mapping

### What Doesn't Work
- ❌ Variable capture - Can't access outer scope
- ❌ Return values - Can't get results back
- ❌ Channels - No communication between spawns
- ❌ Shared state - No safe concurrent access
- ❌ Spawn pools - No worker pool management
- ❌ Async/await - No async syntax

## Proposed Design

### 1. Variable Capture (Critical)

**Syntax:**
```wyn
let x: int = 42
spawn {
    print(x)  // Capture x by value
}

let mut counter: int = 0
spawn {
    counter = counter + 1  // Capture by reference (atomic)
}
```

**Implementation:**
- Compiler: Detect captured variables in spawn block
- Runtime: Pass captured vars as struct to thread function
- Atomic operations for mutable captures

### 2. Spawn Handles & Return Values

**Syntax:**
```wyn
let handle: SpawnHandle[int] = spawn {
    return compute_result()
}

let result: int = handle.join()  // Wait and get result
```

**Implementation:**
```wyn
struct SpawnHandle[T] {
    thread_id: int
    result_ptr: int  // Pointer to result storage
    
    fn join(self) -> T {
        // Wait for thread, return result
    }
    
    fn is_done(self) -> bool {
        // Check if completed without blocking
    }
}
```

### 3. Channels (Go-style)

**Syntax:**
```wyn
let ch: Channel[int] = channel(10)  // Buffered channel

spawn {
    ch.send(42)
}

let value: int = ch.recv()  // Blocking receive
let opt_value: ?int = ch.try_recv()  // Non-blocking
```

**Implementation:**
```wyn
struct Channel[T] {
    buffer: [T]
    capacity: int
    mutex: Mutex
    cond_send: Condition
    cond_recv: Condition
    
    fn send(self, value: T)
    fn recv(self) -> T
    fn try_recv(self) -> ?T
    fn close(self)
}
```

### 4. Mutex & Atomic Operations

**Syntax:**
```wyn
let mut counter: Atomic[int] = atomic(0)

spawn {
    counter.add(1)  // Atomic increment
}

let lock: Mutex = mutex()
lock.lock()
// Critical section
lock.unlock()

// Or with defer
lock.lock()
defer lock.unlock()
// Critical section
```

**Implementation:**
```wyn
struct Atomic[T] {
    value: T
    
    fn get(self) -> T
    fn set(self, value: T)
    fn add(self, delta: T) -> T  // Returns old value
    fn compare_and_swap(self, old: T, new: T) -> bool
}

struct Mutex {
    fn lock(self)
    fn unlock(self)
    fn try_lock(self) -> bool
}
```

### 5. Spawn Pools (Worker Pools)

**Syntax:**
```wyn
let pool: SpawnPool = spawn_pool(4)  // 4 workers

for i in 0..100 {
    pool.submit(fn() {
        process_task(i)
    })
}

pool.wait_all()  // Wait for all tasks
pool.shutdown()
```

**Implementation:**
```wyn
struct SpawnPool {
    workers: int
    task_queue: Channel[fn()]
    
    fn submit(self, task: fn())
    fn wait_all(self)
    fn shutdown(self)
}
```

### 6. Async/Await (Future)

**Syntax:**
```wyn
async fn fetch_data(url: str) -> str {
    let response: str = await http.get(url)
    return response
}

fn main() {
    let future: Future[str] = fetch_data("https://api.example.com")
    let data: str = await future
}
```

**Implementation:**
- Compiler: Transform async functions to state machines
- Runtime: Event loop for async operations
- Integration with spawn for parallelism

## Implementation Plan

### Phase 1: Variable Capture (v1.1)
**Priority: Critical**

1. **Compiler Changes:**
   - Detect captured variables in spawn blocks
   - Generate capture struct
   - Pass struct to thread function

2. **Runtime Changes:**
   - Modify `__wyn_spawn` to accept capture struct
   - Handle value vs reference captures

3. **Tests:**
   - Capture by value
   - Capture by reference
   - Multiple captures
   - Nested captures

**Estimated effort:** 2-3 days

### Phase 2: Spawn Handles & Return Values (v1.1)
**Priority: High**

1. **Add SpawnHandle type:**
   - Store thread ID
   - Store result pointer
   - Implement join()

2. **Compiler changes:**
   - Return SpawnHandle from spawn
   - Type inference for return type

3. **Tests:**
   - Get return values
   - Multiple handles
   - Join order

**Estimated effort:** 2 days

### Phase 3: Channels (v1.2)
**Priority: High**

1. **Implement Channel type:**
   - Buffered and unbuffered
   - Send/receive operations
   - Close semantics

2. **Runtime:**
   - Mutex-based implementation
   - Condition variables
   - Blocking/non-blocking ops

3. **Tests:**
   - Producer-consumer
   - Multiple senders/receivers
   - Channel closing

**Estimated effort:** 3-4 days

### Phase 4: Mutex & Atomics (v1.2)
**Priority: Medium**

1. **Add Mutex type:**
   - Lock/unlock
   - Try-lock
   - Integration with defer

2. **Add Atomic types:**
   - Atomic[int], Atomic[bool]
   - Compare-and-swap
   - Fetch-and-add

3. **Tests:**
   - Concurrent counter
   - Lock contention
   - Atomic operations

**Estimated effort:** 2 days

### Phase 5: Spawn Pools (v1.3)
**Priority: Medium**

1. **Implement SpawnPool:**
   - Worker threads
   - Task queue (using channels)
   - Submit/wait/shutdown

2. **Tests:**
   - Many tasks
   - Different pool sizes
   - Graceful shutdown

**Estimated effort:** 2-3 days

### Phase 6: Async/Await (v2.0)
**Priority: Low (Future)**

1. **Compiler transformation:**
   - State machine generation
   - Await points

2. **Runtime:**
   - Event loop
   - Future type
   - Integration with I/O

**Estimated effort:** 1-2 weeks

## Use Cases Unlocked

### With Variable Capture
```wyn
let urls: [str] = ["url1", "url2", "url3"]
let mut results: [str] = []

for url in urls {
    spawn {
        let data: str = http.get(url)
        results.append(data)  // Safe concurrent append
    }
}
```

### With Channels
```wyn
let ch: Channel[int] = channel(100)

// Producer
spawn {
    for i in 0..1000 {
        ch.send(i)
    }
    ch.close()
}

// Consumer
while true {
    let val: ?int = ch.try_recv()
    if val == none { break }
    process(val!)
}
```

### With Spawn Pools
```wyn
let pool: SpawnPool = spawn_pool(8)

for file in files {
    pool.submit(fn() {
        process_file(file)
    })
}

pool.wait_all()
```

### Parallel Test Runner (Future)
```wyn
let pool: SpawnPool = spawn_pool(cpu_count())
let results: Channel[TestResult] = channel(100)

for test in tests {
    pool.submit(fn() {
        let result: TestResult = run_test(test)
        results.send(result)
    })
}

let mut passed: int = 0
for i in 0..tests.len() {
    let result: TestResult = results.recv()
    if result.passed {
        passed = passed + 1
    }
}
```

## Performance Impact

### Variable Capture
- Minimal overhead (struct allocation)
- Same speed as manual passing

### Channels
- Mutex overhead per operation
- Buffered channels reduce contention
- Comparable to Go channels

### Spawn Pools
- Eliminates thread creation overhead
- Better for many small tasks
- 2-3x faster than spawning per task

## Compatibility

### Go Comparison
After full implementation, Wyn will have:
- ✅ Goroutines → spawn
- ✅ Channels → Channel[T]
- ✅ Mutex → Mutex
- ✅ Atomic → Atomic[T]
- ✅ WaitGroup → SpawnPool.wait_all()

**Wyn will match Go's concurrency model with better performance.**

## Recommendation

**Implement in order:**
1. **Variable capture** (v1.1) - Unlocks 80% of use cases
2. **Spawn handles** (v1.1) - Get return values
3. **Channels** (v1.2) - Communication between spawns
4. **Mutex/Atomics** (v1.2) - Shared state safety
5. **Spawn pools** (v1.3) - Performance optimization
6. **Async/await** (v2.0) - Ergonomic async I/O

**Start with Phase 1 (variable capture) - it's the foundation for everything else.**
