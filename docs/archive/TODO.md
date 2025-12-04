# TODO List for v1.0

## Critical (Blocking Release)

### 1. Variable Capture for spawn 🔴
**Status**: Not working
**Issue**: Threads can't access outer scope variables
**Solution**: Implement heap-allocated context struct
**Effort**: 1-2 days
**Assign**: Agent

### 2. Test Atomic Operations 🔴
**Status**: Infrastructure complete, not tested
**Issue**: Can't test without variable capture
**Solution**: Wait for #1
**Effort**: 1 hour
**Assign**: Manual after #1

## High Priority (Nice to Have)

### 3. Complete Benchmark Suite
**Status**: Partial
**Missing**: I/O, network, more concurrency tests
**Effort**: 2-3 hours
**Assign**: Agent

### 4. Documentation Polish
**Status**: Good but scattered
**Need**: Clean up docs/, consolidate guides
**Effort**: 1-2 hours
**Assign**: Manual

## Medium Priority

### 5. Loop Unrolling
**Status**: Structure added, not implemented
**Effort**: 2-3 hours
**Assign**: Agent

### 6. Better Error Messages
**Status**: Good, could be better
**Effort**: 1-2 hours
**Assign**: Manual

## Current Blockers

**Main blocker**: Variable capture for spawn

Without this, spawn can't:
- Use loop variables
- Share counters
- Access outer scope

**Everything else is ready for v1.0.**

## Recommendation

**Option A**: Implement variable capture (2-3 days)
- Full goroutine parity
- spawn production-ready
- Can rival Go completely

**Option B**: Ship v1.0 without full spawn (today)
- Document spawn limitations
- Excellent performance on everything else
- Add spawn in v1.1

## Decision Needed

Which path to take?
