# task Module

std/task.wyn
Task channels for communication between spawn blocks
Task channel type (opaque pointer to Channel*)
Use for sending/receiving values between concurrent tasks
Create a new task channel

## Functions

### `new() -> int`

### `send(ch: int, value: int)`

### `recv(ch: int) -> int`

### `close(ch: int)`

### `free(ch: int)`

