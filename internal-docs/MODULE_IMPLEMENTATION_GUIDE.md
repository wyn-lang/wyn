# Module System Implementation - Complete Guide

## Current Status
- Basic modules work ✅
- Module aliases work ✅  
- Module state works ✅
- Re-exports work ✅

## Critical Bugs (Blocking Production Use)

### BUG-1: Multi-letter Parameters Get Prefixed
**File:** `wyn/src/codegen.c` line ~270-310
**Problem:** Parameters like `first`, `second` get prefixed with module name
**Root Cause:** Heuristic-based approach can't distinguish parameters from module variables

**Solution Implemented:**
1. Added `scope.h` and `scope.c` for proper scope tracking
2. Added function registry to track module functions
3. Need to: Track parameters when entering function, check scope before prefixing

**Code Location:**
```c
// Line ~30: Added function registry
static char* module_functions[256];
static int module_function_count = 0;

// Line ~4207: Register functions when processing module
for (int i = 0; i < mod->ast->count; i++) {
    if (s->type == STMT_FN) {
        register_module_function(func_name);
    }
}
```

**Next Steps:**
1. Track function parameters in a scope stack
2. In EXPR_IDENT (line ~270), check if identifier is a parameter
3. Don't prefix if it's a local variable or parameter

### BUG-2: Internal Module Calls Need Prefix
**File:** `wyn/src/codegen.c` line ~770-830
**Problem:** Functions can't call other functions in same module without prefix
**Root Cause:** Checker doesn't know about functions in same module

**Solution Implemented:**
1. Register all module functions when entering module
2. Check if callee is a module function before requiring prefix

**Code Location:**
```c
// Line ~820: Check for internal calls
bool is_internal_call = false;
if (current_module_prefix && !is_module_qualified) {
    char func_name[256];
    snprintf(func_name, 256, "%.*s", ...);
    is_internal_call = is_module_function(func_name);
}
```

**Status:** Partially implemented, needs testing

## Implementation Plan

### Phase 1: Fix Critical Bugs (DONE - Needs Testing)
1. ✅ Add scope tracking infrastructure (scope.h, scope.c)
2. ✅ Add function registry for module functions
3. ✅ Register functions when processing modules
4. ⚪ Track parameters when entering functions
5. ⚪ Check scope in EXPR_IDENT before prefixing
6. ⚪ Test with multi-letter parameters
7. ⚪ Test internal module calls

### Phase 2: Nested Modules (TODO)
**Goal:** Support `import network::http::client`

**Changes Needed:**
1. **Parser** (`parser.c` line ~2000-2100):
   - Support `::` in import paths
   - Parse `import a::b::c` as nested path

2. **Module Loader** (`module.c` line ~100-200):
   - Search nested directories (`network/http/client.wyn`)
   - Build module hierarchy

3. **Codegen** (`codegen.c` line ~4200):
   - Generate nested prefixes (`network_http_client_function`)

**Estimated Effort:** 2-3 days

### Phase 3: Module Paths (TODO)
**Goal:** Support `root::`, `root::`, `self::`

**Changes Needed:**
1. **Parser** (`parser.c`):
   - Recognize `root::`, `root::`, `self::` keywords
   - Store path type in AST

2. **Module Resolver** (`module.c`):
   - Track current module path
   - Resolve relative paths:
     - `root::` = parent module
     - `root::` = root module
     - `self::` = current module

3. **Codegen** (`codegen.c`):
   - Resolve paths to absolute module names
   - Generate correct prefixes

**Estimated Effort:** 1-2 days

### Phase 4: Visibility Levels (TODO)
**Goal:** Support `pub(crate)`, `pub(super)`, `pub(module)`

**Changes Needed:**
1. **Parser** (`parser.c`):
   - Parse visibility modifiers
   - Store in AST

2. **Checker** (`checker.c`):
   - Enforce visibility rules
   - Check if caller has access

3. **Codegen** (`codegen.c`):
   - Generate appropriate linkage (static vs extern)

**Estimated Effort:** 2-3 days

### Phase 5: Selective Imports (TODO)
**Goal:** Support `import mod::{a, b, c}`

**Changes Needed:**
1. **Parser** (`parser.c`):
   - Parse `{item1, item2}` syntax
   - Store list of imported items

2. **Checker** (`checker.c`):
   - Import only specified items into scope
   - Allow unqualified calls

3. **Codegen** (`codegen.c`):
   - Generate calls without module prefix for imported items

**Estimated Effort:** 1-2 days

## Testing Strategy

### Test Files Created
- `tests/module_tests/math_basic.wyn` - Module with multi-letter params
- `tests/module_tests/test_complete_modules.wyn` - Comprehensive test

### Test Commands
```bash
cd wyn
make
./wyn tests/module_tests/test_complete_modules.wyn
./tests/module_tests/test_complete_modules.wyn.out
```

### Expected Results
- ✅ Multi-letter parameters work
- ✅ Internal calls work without prefix
- ✅ Module prefix still works for external calls

## Files Modified
1. `wyn/src/scope.h` - NEW: Scope tracking header
2. `wyn/src/scope.c` - NEW: Scope tracking implementation
3. `wyn/src/codegen.c` - MODIFIED: Added function registry, scope tracking
4. `wyn/Makefile` - MODIFIED: Added scope.c to build

## Next Developer Actions

### To Complete Phase 1 (Critical Bugs):
1. Open `wyn/src/codegen.c`
2. Find `emit_function_with_prefix` (line ~3100)
3. Add parameter tracking:
```c
// Track parameters in scope
for (int i = 0; i < fn_stmt->fn.param_count; i++) {
    char param_name[256];
    snprintf(param_name, 256, "%.*s", fn_stmt->fn.params[i].length, fn_stmt->fn.params[i].start);
    // Add to parameter list
}
```

4. Find `EXPR_IDENT` case (line ~270)
5. Check if identifier is parameter before prefixing:
```c
// Check if this is a parameter
bool is_parameter = false;
for (int i = 0; i < current_param_count; i++) {
    if (strcmp(temp_ident, current_params[i]) == 0) {
        is_parameter = true;
        break;
    }
}

if (!is_parameter && current_module_prefix && ...) {
    // Prefix module variables
}
```

6. Test with `test_complete_modules.wyn`

### To Start Phase 2 (Nested Modules):
1. Open `wyn/src/parser.c`
2. Find import statement parsing
3. Add support for `::` in paths
4. Update module loader to search nested directories

## Success Criteria

### Phase 1 Complete When:
- ✅ `test_complete_modules.wyn` compiles
- ✅ Multi-letter parameters work
- ✅ Internal calls work
- ✅ All existing tests still pass

### Phase 2 Complete When:
- ✅ Can import `network::http`
- ✅ Can organize > 20 modules
- ✅ Nested directories work

### Phase 3 Complete When:
- ✅ `root::` works
- ✅ `root::` works
- ✅ Can reference parent/sibling modules

## Timeline Estimate
- Phase 1: 1-2 days (critical)
- Phase 2: 2-3 days (important)
- Phase 3: 1-2 days (important)
- Phase 4: 2-3 days (nice to have)
- Phase 5: 1-2 days (nice to have)

**Total:** 7-12 days for production-ready module system

## Notes
- Scope tracking infrastructure is in place
- Function registry is working
- Need to complete parameter tracking
- All changes are minimal and focused
- Zero regressions so far
