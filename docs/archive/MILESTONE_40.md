# Milestone: 40% Language Coverage
**Target Date:** 2025-12-09
**Status:** 🚧 In Progress

## Goal

Reach 40% language coverage by adding 4 more features to Stage 2:

1. ✅ Match expressions (proven in stage2_match.wyn)
2. ✅ Enums (proven in stage2_enums.wyn)
3. ✅ Error handling (proven in stage2_errors.wyn)
4. ✅ Closures (proven in stage2_closures.wyn)

## Current Status

### Completed (35% - 12 features)
- ✅ Variables
- ✅ Expressions
- ✅ If/Else-if
- ✅ While loops
- ✅ For loops
- ✅ Functions
- ✅ Arrays
- ✅ Structs
- ✅ Returns
- ✅ Break/Continue
- ✅ Strings

### Proven but Not Integrated (4 features)
- ⏳ Match expressions
- ⏳ Enums
- ⏳ Error handling
- ⏳ Closures

## Integration Plan

### Step 1: Create Unified Stage 2
Create `src/stage1/stage2.wyn` that:
- Reads input Wyn files
- Parses them (simple lexer)
- Generates ARM64 assembly
- Supports all 16 features

### Step 2: Add Real Parsing
Instead of hardcoded assembly:
- Tokenize input
- Parse statements
- Build simple AST
- Generate code from AST

### Step 3: Test All Features
Verify each feature works:
```bash
./build/stage2 examples/match_test.wyn
./build/stage2 examples/enum_test.wyn
./build/stage2 examples/error_test.wyn
./build/stage2 examples/closure_test.wyn
```

## Feature Details

### Match Expressions
```wyn
let x: int = 2
match x {
    1 => print("One")
    2 => print("Two")
    _ => print("Other")
}
```

**Assembly Pattern:**
```asm
cmp x19, #1
beq .L_case1
cmp x19, #2
beq .L_case2
b .L_default
```

### Enums
```wyn
enum Color {
    Red
    Green
    Blue
}

let c: Color = Color.Red
```

**Assembly Pattern:**
```asm
mov x19, #0  # Red = 0
mov x19, #1  # Green = 1
mov x19, #2  # Blue = 2
```

### Error Handling
```wyn
fn safe_div(a: int, b: int) -> (int, int) {
    if b == 0 {
        return (0, 1)  # error
    }
    return (a / b, 0)  # ok
}

let (result, err): (int, int) = safe_div(10, 2)
if err == 0 {
    print("Success")
}
```

**Assembly Pattern:**
```asm
cmp x1, #0
beq .L_error
udiv x0, x0, x1
mov x1, #0
ret
.L_error:
mov x0, #0
mov x1, #1
ret
```

### Closures
```wyn
fn make_adder(x: int) -> fn(int) -> int {
    return fn(y: int) -> int {
        return x + y
    }
}

let add5: fn(int) -> int = make_adder(5)
let result: int = add5(3)  # 8
```

**Assembly Pattern:**
```asm
# Capture x in closure
str x19, [sp, #0]
# Call closure
ldr x19, [sp, #0]
add x0, x19, x0
```

## Test Programs

### Match Test
```wyn
fn main() {
    let x: int = 2
    match x {
        1 => print("One")
        2 => print("Two")
        _ => print("Other")
    }
}
```

### Enum Test
```wyn
enum Status {
    Ok
    Error
}

fn main() {
    let s: Status = Status.Ok
    if s == Status.Ok {
        print("Success")
    }
}
```

### Error Test
```wyn
fn safe_div(a: int, b: int) -> (int, int) {
    if b == 0 {
        return (0, 1)
    }
    return (a / b, 0)
}

fn main() {
    let (r, e): (int, int) = safe_div(10, 0)
    if e == 0 {
        print("Ok")
    } else {
        print("Error")
    }
}
```

### Closure Test
```wyn
fn make_counter() -> fn() -> int {
    let count: int = 0
    return fn() -> int {
        count = count + 1
        return count
    }
}

fn main() {
    let counter: fn() -> int = make_counter()
    print(counter())  # 1
    print(counter())  # 2
}
```

## Success Criteria

- [ ] Stage 2 reads and parses input files
- [ ] Match expressions work
- [ ] Enums work
- [ ] Error handling works
- [ ] Closures work
- [ ] All 16 features integrated
- [ ] 40% coverage achieved
- [ ] Self-hosting maintained

## Timeline

- **Hour 1:** Create unified Stage 2 with parsing
- **Hour 2:** Integrate match + enums
- **Hour 3:** Integrate errors + closures
- **Hour 4:** Test and verify 40%

## Next Steps After 40%

### 45% Coverage (5 more features)
- [ ] Traits
- [ ] Generics
- [ ] Type inference
- [ ] Multiple return values
- [ ] Destructuring

### 50% Coverage (Milestone!)
- [ ] Real imports
- [ ] Modules
- [ ] Packages
- [ ] Compile Stage 1 subset

---

**Status:** 🚧 In Progress
**Current:** 35%
**Target:** 40%
**ETA:** 2 hours
