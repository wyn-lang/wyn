# Concurrency in Wyn

Wyn provides simple, safe concurrency through the `spawn` keyword.

## Basic Usage

```wyn
let counter: int = 0
for i in 0..100 {
    spawn {
        counter = counter + 1
    }
}
// Spawns auto-join at scope end
print(counter)  // 100
```

## Key Features

- **Auto-join**: Spawns automatically wait at scope end
- **Variable capture**: Spawns can capture and modify variables from outer scope
- **Thread-safe**: Built-in synchronization for shared variables
- **Simple**: No explicit locks, mutexes, or channels needed

## Arrays in Spawns

Arrays can be safely shared across spawns:

```wyn
let arr: [int] = []
for i in 0..10 {
    spawn {
        arr.append(i)
    }
}
print(arr.len())  // 10
```

## Limitations

- Spawns capture variables by reference
- All captured variables must be in scope when spawn executes
- Nested spawns are supported

## Implementation

Wyn uses platform threads with automatic synchronization for captured variables.
