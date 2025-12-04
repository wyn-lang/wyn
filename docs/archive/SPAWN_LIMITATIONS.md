# spawn Current Limitations

## What Works ✅
- spawn creates real threads (pthread)
- Threads execute in parallel
- Auto-join at function exit
- No crashes or hangs

## Critical Issue ❌

### Variable Capture Not Working
```wyn
let mut i = 0
while i < 5 {
    spawn {
        work(i)  // i is not captured correctly!
    }
    i = i + 1
}
```

**Problem**: spawn doesn't capture variables from outer scope. Each thread sees garbage values.

**Root Cause**: spawn body needs to capture variables, but current codegen doesn't implement closures.

## Workaround

### Works: No External Variables
```wyn
spawn {
    print_str("Hello")  // OK - no variables
}
```

### Doesn't Work: Uses External Variables
```wyn
let x = 42
spawn {
    print(x)  // BROKEN - x not captured
}
```

## To Fix

Need to implement variable capture:
1. Detect which variables are used in spawn body
2. Allocate heap memory for captured variables
3. Pass pointer to thread function
4. Access captured variables through pointer

**Effort**: 2-3 days (complex)

## Decision

### Option A: Fix Variable Capture
- Implement closures for spawn
- 2-3 days of work
- Complex and risky

### Option B: Document Limitation
- spawn works for independent tasks
- No variable capture in v1.0
- Add in v1.1

## Recommendation

**Document limitation, ship v1.0 without variable capture.**

spawn is useful for:
- Independent parallel tasks
- I/O operations
- Network requests
- Any work that doesn't need shared state

**Not useful for**:
- Parallel computation with shared state
- Capturing loop variables
- Shared counters/accumulators

## Current Status

- ✅ pthread working
- ✅ True parallelism
- ✅ Auto-join
- ❌ Variable capture missing
- ❌ Can't rival Go goroutines yet

**spawn needs variable capture to be production-ready.**
