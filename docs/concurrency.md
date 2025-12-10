# Wyn Concurrency - Go-Style Goroutines

## Goal

Support **millions of concurrent tasks** like Go's goroutines with:
- Non-blocking execution
- Lightweight green threads
- Work-stealing scheduler
- Automatic load balancing

## Architecture

### M:N Threading Model

```
Millions of Tasks (M)
        ↓
Work-Stealing Scheduler
        ↓
N OS Threads (Workers)
```

- **M green threads** (tasks) mapped to **N OS threads** (workers)
- Each worker has a local run queue
- Workers steal tasks from each other when idle
- Automatic worker count based on CPU cores

### Components

1. **Task** - Lightweight execution unit
   - Function pointer
   - Small stack (4-8KB)
   - State (ready, running, blocked)
   - Context for switching

2. **Worker** - OS thread that executes tasks
   - Local run queue (lock-free)
   - Steals from other workers when idle
   - Sleeps when no work available

3. **Scheduler** - Distributes tasks to workers
   - Global task queue
   - Work-stealing algorithm
   - Load balancing

## Language Syntax

### Spawn Blocks

```wyn
spawn {
    print("Hello from task")
    let x: int = compute()
}
```

Compiles to:
```llvm
define void @__spawn_0() {
    ; task body
    ret void
}

call i64 @__wyn_spawn(i8* @__spawn_0, i8* null)
```

### Async Functions

```wyn
async fn fetch_data(url: str) -> str {
    # Non-blocking I/O
    return http.get(url)
}

fn main() {
    let result: str = await fetch_data("https://api.example.com")
    print(result)
}
```

## Runtime API

### Core Functions

```c
// Initialize runtime (called at program start)
void __wyn_runtime_init();

// Shutdown runtime (called at program end)
void __wyn_runtime_shutdown();

// Spawn a new task
long long __wyn_spawn(void* func_ptr, void* context);

// Yield to scheduler
void __wyn_yield();

// Wait for async result
long long __wyn_await(void* future);
```

## Implementation Status

### ✅ Compiler Support (Complete)
- [x] Parse spawn blocks
- [x] Parse async functions
- [x] LLVM codegen for spawn
- [x] Generate spawn functions
- [x] Runtime init/shutdown calls
- [x] String collection for spawn blocks

### ⏳ Runtime (In Progress - Agent)
- [ ] M:N scheduler implementation
- [ ] Work-stealing queues
- [ ] Context switching (setjmp/longjmp or ucontext)
- [ ] Worker thread pool
- [ ] Task allocation and management
- [ ] Async I/O integration

### 📋 TODO
- [ ] Await expression codegen
- [ ] Future/Promise types
- [ ] Channel communication
- [ ] Select statement for channels
- [ ] Async I/O (epoll/kqueue)

## Performance Goals

- **Spawn overhead**: < 100ns per task
- **Context switch**: < 10ns
- **Memory per task**: 4-8KB stack
- **Scalability**: 1M+ concurrent tasks
- **Throughput**: Match or exceed Go runtime

## Testing

### Basic Test (10 tasks)
```bash
./build/wync examples/spawn_test.wyn
./a.out
```

### Stress Test (1M tasks)
```bash
./build/wync examples/spawn_million.wyn
./a.out
```

## References

- Go runtime scheduler design
- Work-stealing algorithms (Chase-Lev deque)
- Tokio (Rust async runtime)
- Project Loom (Java virtual threads)
