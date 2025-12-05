# Bug Fix: Spawn Inside Loops on ARM64

## Problem

Spawn statements inside loops were reading garbage values on ARM64 instead of the correct captured variable values.

**Symptoms:**
- Spawn OUTSIDE loops: ✅ Works perfectly (x = 100, spawn sees 100, result = 101)
- Spawn INSIDE loops: ❌ Reads garbage (x = 100, spawn sees 14)

## Root Cause

The `analyze_spawn_captures()` function only searched for spawn statements at the top level of function bodies. It did not recursively search inside:
- For loops
- While loops  
- If/else blocks

When a spawn was inside a loop, the captured variables were never detected, so:
1. No context was allocated (`cap_count = 0`)
2. No variable addresses were passed to the thread
3. The thread function tried to read uninitialized memory

## Solution

Created a recursive helper function `analyze_spawn_stmt()` that:
1. Searches for spawn statements recursively through all nested control structures
2. Analyzes captured variables when a spawn is found
3. Recursively processes for loops, while loops, and if/else blocks

## Code Changes

**File:** `bootstrap/stage0.c` (lines 4237-4280)

**Before:**
```c
static void analyze_spawn_captures(Module* m) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            Stmt* s = fn->body[j];
            if (s->kind == STMT_SPAWN) {
                // Only found top-level spawns
            }
        }
    }
}
```

**After:**
```c
static void analyze_spawn_stmt(Stmt* s) {
    if (!s) return;
    
    if (s->kind == STMT_SPAWN) {
        // Analyze captured variables
        char used_vars[64][MAX_IDENT_LEN];
        int used_count = 0;
        
        for (int k = 0; k < s->spawn.body_count; k++) {
            find_used_vars_stmt(s->spawn.body[k], used_vars, &used_count, 64);
        }
        
        if (used_count > 0) {
            s->spawn.captured_vars = malloc(sizeof(char*) * used_count);
            s->spawn.captured_count = used_count;
            for (int k = 0; k < used_count; k++) {
                s->spawn.captured_vars[k] = strdup(used_vars[k]);
            }
        }
        return;
    }
    
    // Recursively search in nested statements
    switch (s->kind) {
        case STMT_FOR:
            for (int i = 0; i < s->for_stmt.body_count; i++)
                analyze_spawn_stmt(s->for_stmt.body[i]);
            break;
        case STMT_WHILE:
            for (int i = 0; i < s->while_stmt.body_count; i++)
                analyze_spawn_stmt(s->while_stmt.body[i]);
            break;
        case STMT_IF:
            for (int i = 0; i < s->if_stmt.then_count; i++)
                analyze_spawn_stmt(s->if_stmt.then_block[i]);
            for (int i = 0; i < s->if_stmt.else_count; i++)
                analyze_spawn_stmt(s->if_stmt.else_block[i]);
            break;
        default: break;
    }
}

static void analyze_spawn_captures(Module* m) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            analyze_spawn_stmt(fn->body[j]);
        }
    }
}
```

## Test Results

All 85 tests pass, including all 11 spawn tests:
- ✅ atomic_spawn_test.wyn
- ✅ spawn_simple.wyn
- ✅ spawn_capture_test.wyn
- ✅ spawn_shared_counter_test.wyn
- ✅ spawn_shared_array_test.wyn
- ✅ spawn_multiple_vars_test.wyn
- ✅ spawn_nested_capture_test.wyn
- ✅ spawn_immutable_test.wyn
- ✅ spawn_minimal_test.wyn
- ✅ spawn_race_condition_test.wyn
- ✅ spawn_atomic_comprehensive_test.wyn

## Verification

Test case that now works correctly:
```wyn
fn main() {
    let mut x: int = 100
    for i in 0..2 {
        spawn { x = x + 1 }  // Now correctly sees x = 100, not garbage
    }
}
```

Generated assembly now correctly:
1. Allocates context: `mov x0, #8; bl _malloc`
2. Stores variable address: `add x1, x29, #24; str x1, [x19, #0]`
3. Passes context to spawn: `mov x1, x19; bl ___wyn_spawn`
4. Thread loads from correct address: `ldr x1, [x0, #0]; ldar x0, [x1]`

## Impact

This fix enables spawn to work correctly in all contexts:
- ✅ Top-level spawn statements
- ✅ Spawn inside for loops
- ✅ Spawn inside while loops
- ✅ Spawn inside if/else blocks
- ✅ Spawn in nested control structures

This was the final blocker for v1.0.0.
