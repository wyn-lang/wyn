# Wyn Standard Library Reference

Complete reference for all standard library functions.

## Table of Contents

1. [I/O Functions](#io-functions)
2. [String Functions](#string-functions)
3. [Math Functions](#math-functions)
4. [Collections](#collections)
5. [File Operations](#file-operations)
6. [JSON](#json)
7. [Concurrency](#concurrency)
8. [Async](#async)

## I/O Functions

### print(value: int)
Prints an integer followed by newline.
```wyn
print(42);  // Output: 42
```

### print_str(text: string)
Prints a string followed by newline.
```wyn
print_str("Hello");  // Output: Hello
```

### print_float(value: float)
Prints a floating point number.
```wyn
print_float(3.14);  // Output: 3.14
```

### input() -> int
Reads an integer from standard input.
```wyn
let num = input();
```

## String Functions

### str_len(s: string) -> int
Returns the length of a string.
```wyn
let len = str_len("hello");  // Returns 5
```

### str_concat(a: string, b: string) -> string
Concatenates two strings.
```wyn
let result = str_concat("Hello", " World");
```

### str_eq(a: string, b: string) -> bool
Compares two strings for equality.
```wyn
if str_eq(name, "Alice") {
    print_str("Found Alice");
}
```

## Math Functions

### abs_val(x: int) -> int
Returns absolute value.
```wyn
let result = abs_val(-5);  // Returns 5
```

### min(a: int, b: int) -> int
Returns minimum of two values.
```wyn
let smaller = min(10, 20);  // Returns 10
```

### max(a: int, b: int) -> int
Returns maximum of two values.
```wyn
let larger = max(10, 20);  // Returns 20
```

### pow_int(base: int, exp: int) -> int
Raises base to the power of exp.
```wyn
let result = pow_int(2, 3);  // Returns 8
```

## Collections

### HashMap

#### hashmap_new() -> WynHashMap*
Creates a new hash map.
```wyn
let map = hashmap_new();
```

#### hashmap_insert(map: WynHashMap*, key: string, value: int)
Inserts a key-value pair.
```wyn
hashmap_insert(map, "age", 25);
```

#### hashmap_get(map: WynHashMap*, key: string) -> int
Gets a value by key. Returns -1 if not found.
```wyn
let age = hashmap_get(map, "age");
```

#### hashmap_free(map: WynHashMap*)
Frees the hash map.
```wyn
hashmap_free(map);
```

### HashSet

#### hashset_new() -> WynHashSet*
Creates a new hash set.
```wyn
let set = hashset_new();
```

#### hashset_add(set: WynHashSet*, key: string)
Adds an element to the set.
```wyn
hashset_add(set, "apple");
```

#### hashset_contains(set: WynHashSet*, key: string) -> int
Checks if element exists. Returns 1 if found, 0 otherwise.
```wyn
if hashset_contains(set, "apple") == 1 {
    print_str("Found");
}
```

#### hashset_remove(set: WynHashSet*, key: string)
Removes an element from the set.
```wyn
hashset_remove(set, "apple");
```

#### hashset_free(set: WynHashSet*)
Frees the hash set.
```wyn
hashset_free(set);
```

## File Operations

### file_read(path: string) -> string
Reads entire file contents.
```wyn
let content = file_read("/tmp/data.txt");
```

### file_write(path: string, content: string) -> int
Writes content to file. Returns 1 on success, 0 on failure.
```wyn
let result = file_write("/tmp/output.txt", "Hello");
```

### file_append(path: string, content: string) -> int
Appends content to file. Returns 1 on success.
```wyn
file_append("/tmp/log.txt", "New entry\n");
```

### file_exists(path: string) -> int
Checks if file exists. Returns 1 if exists, 0 otherwise.
```wyn
if file_exists("/tmp/config.txt") == 1 {
    print_str("Config found");
}
```

## JSON

### json_parse(text: string) -> WynJson*
Parses JSON string into object.
```wyn
let json = json_parse("{\"name\":\"Alice\",\"age\":30}");
```

### json_get_string(json: WynJson*, key: string) -> string
Gets string value from JSON object.
```wyn
let name = json_get_string(json, "name");
```

### json_get_int(json: WynJson*, key: string) -> int
Gets integer value from JSON object.
```wyn
let age = json_get_int(json, "age");
```

### json_free(json: WynJson*)
Frees JSON object.
```wyn
json_free(json);
```

## Concurrency

### spawn(fn: function) -> thread
Spawns a new thread.
```wyn
spawn(worker_function);
```

### channel_new() -> Channel
Creates a new channel for communication.
```wyn
let ch = channel_new();
```

## Async

### async fn
Declares an asynchronous function.
```wyn
async fn fetch() -> int {
    return 42;
}
```

### await
Waits for async operation to complete.
```wyn
let future = fetch();
let result = await future;
```

## Utility Functions

### assert_eq(a: int, b: int)
Asserts two values are equal.
```wyn
assert_eq(result, 42);
```

### panic(message: string)
Terminates program with error message.
```wyn
if error {
    panic("Fatal error occurred");
}
```

### exit_program(code: int)
Exits program with status code.
```wyn
exit_program(1);
```

## See Also

- [Language Guide](LANGUAGE_GUIDE.md)
- [Build Guide](BUILD_GUIDE.md)
- [Module Guide](MODULE_GUIDE.md)
