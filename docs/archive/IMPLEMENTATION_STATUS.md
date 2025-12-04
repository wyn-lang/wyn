# spawn Variable Capture - Implementation Status

## Goal
Enable spawn to capture variables from outer scope, like Go goroutines.

## Current State

### What Works ✅
- spawn creates real threads
- True parallelism
- Auto-join at function exit
- Independent tasks work perfectly

### What's Broken ❌
- Variable capture (threads see garbage values)
- Shared state (race conditions)

## Implementation Plan

### Phase 1: Variable Capture Analysis (Done)
- ✅ Added `find_used_vars_expr/stmt()` functions
- ✅ Added `analyze_spawn_captures()` to detect captured vars
- ✅ Added `captured_vars` array to spawn struct
- ✅ Integrated into optimization pipeline

### Phase 2: Context Struct Generation (In Progress)
Need to:
1. Generate context struct with captured variables
2. Allocate context on heap
3. Copy variables to context
4. Pass context to thread function
5. Access variables through context in thread

### Phase 3: Atomic Operations (Agent Working)
- Detect shared variables
- Generate atomic load/store
- Prevent race conditions

## Complexity

Variable capture requires:
- Heap allocation for context
- Pointer passing to threads
- Variable substitution in spawn body
- Proper memory management

**Estimated**: 300-400 lines of complex codegen

## Alternative: Simple Solution

Instead of full capture, support only:
1. **Pass by value** - Copy variables at spawn time
2. **No shared state** - Each thread gets own copy
3. **Document limitation** - No shared counters

This is simpler (100 lines) and covers 80% of use cases.

## Recommendation

Implement simple pass-by-value capture:
- Copy variables at spawn time
- Each thread gets independent copy
- No shared state (document limitation)
- Ship v1.0 with this

Full closure capture can be v1.1.

## Current Blockers

1. Codegen complexity (context struct generation)
2. Memory management (heap allocation)
3. Variable substitution in spawn body

**Need 1-2 days to complete properly.**
