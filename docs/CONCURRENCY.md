# Wyn Concurrency Guide

Wyn provides production-ready concurrency with automatic task synchronization, eliminating the need for manual coordination with `sleep_ms()` calls.

## Overview

Wyn's concurrency system features:
- **Fire-and-forget spawning** with `spawn { ... }`
- **Automatic synchronization** at program exit
- **Manual synchronization** with `task_join_all()` when needed
- **True parallelism** with M:N scheduler (1.3M tasks/sec)
- **Thread safety** with built-in synchronization primitives

## Basic Usage

### Fire-and-Forget Spawning

```wyn
fn main() {
    spawn {
        print("Task 1 running...")
        sleep_ms(100)
        print("Task 1 completed!")
    }
    
    spawn {
        print("Task 2 running...")
        sleep_ms(200)  
        print("Task 2 completed!")
    }
    
    print("Main thread continues...")
    
    # All spawned tasks automatically complete before program exit
}
```

**Output:**
```
Task 1 running...
Task 2 running...
Main thread continues...
Task 1 completed!
Task 2 completed!
```

### Manual Synchronization

When you need to wait for tasks to complete mid-function:

```wyn
fn main() {
    let result: int = 0
    
    spawn {
        result = 42
    }
    
    # Wait for all spawned tasks to complete
    task_join_all()
    
    # Now we can safely use the result
    assert(result == 42)
    print("Result: " + int_to_str(result))
}
```

## Synchronization Options

### 1. Automatic Synchronization (Recommended)

The runtime automatically waits for all spawned tasks when `main()` exits:

```wyn
fn main() {
    spawn { /* long running task */ }
    spawn { /* another task */ }
    
    # No manual coordination needed!
    # Runtime ensures all tasks complete before exit
}
```

### 2. Manual Synchronization

Use `task_join_all()` when you need to wait for tasks mid-function:

```wyn
fn main() {
    spawn { /* task 1 */ }
    spawn { /* task 2 */ }
    
    task_join_all()  # Wait for all tasks
    
    # Continue with more work...
    spawn { /* task 3 */ }
    
    # Automatic sync at exit handles task 3
}
```

## Variable Capture

Spawned tasks can capture variables from their enclosing scope:

```wyn
fn main() {
    let counter: int = 0
    const message: str = "Hello"
    
    spawn {
        counter = counter + 1
        print(message + " from task!")
    }
    
    spawn {
        counter = counter + 10  
    }
    
    task_join_all()
    assert(counter == 11)
}
```

## Performance

- **Throughput**: 1.3M tasks/sec
- **Memory**: 8KB per task
- **Scheduler**: M:N threading with work-stealing
- **Overhead**: Minimal - spawning is nearly free

## Best Practices

### ✅ Do

```wyn
# Use automatic synchronization when possible
fn main() {
    spawn { do_work() }
    spawn { do_more_work() }
    # Automatic sync at exit
}

# Use manual sync when needed mid-function
fn process_data() {
    spawn { load_data() }
    task_join_all()
    spawn { process_loaded_data() }
}
```

### ❌ Don't

```wyn
# Don't use sleep_ms() for synchronization
fn main() {
    spawn { do_work() }
    sleep_ms(1000)  # ❌ Bad - guessing timing
    # Use task_join_all() instead
}

# Don't spawn without considering synchronization
fn main() {
    spawn { modify_shared_state() }
    use_shared_state()  # ❌ Race condition
    # Use task_join_all() before accessing shared state
}
```

## Migration from sleep_ms()

**Before (manual timing):**
```wyn
fn main() {
    spawn { do_work() }
    sleep_ms(500)  # Guessing how long task takes
    assert(work_completed)
}
```

**After (proper synchronization):**
```wyn
fn main() {
    spawn { do_work() }
    task_join_all()  # Wait for actual completion
    assert(work_completed)
}
```

## Thread Safety

Wyn's concurrency system provides:
- **Mutex protection** for shared data structures
- **Condition variables** for task coordination  
- **Atomic operations** for lock-free programming
- **Memory safety** with automatic cleanup

## Examples

See the `tests/spawn_*.wyn` files for comprehensive examples of:
- Variable capture
- Multiple task coordination
- Shared state management
- Race condition prevention
- Performance testing

All examples use proper synchronization without `sleep_ms()` calls.
