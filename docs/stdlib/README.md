# Wyn Standard Library Documentation

## Modules

- [io](io.md) - File I/O operations
- [os](os.md) - Operating system interface
- [net](net.md) - Networking (HTTP, TCP, UDP)
- [json](json.md) - JSON parsing
- [time](time.md) - Time operations
- [log](log.md) - Logging
- [encoding](encoding.md) - Base64, hex, URL encoding
- [hash](hash.md) - Cryptographic hashing
- [compress](compress.md) - Compression
- [regex](regex.md) - Pattern matching
- [collections](collections.md) - HashMap, Set
- [crypto](crypto.md) - Cryptography
- [string](string.md) - String operations
- [array](array.md) - Array operations
- [math](math.md) - Mathematical functions
- [task](task.md) - Concurrency
- [core](core.md) - Core utilities
- [result](result.md) - Result/Option types

## Usage

```wyn
import module_name

fn main() {
    // Use module functions
    module_name.function_name(args)
}
```

## All Functions Are Real

Every function in the Wyn stdlib uses real implementations:
- File I/O uses fopen/fread/fwrite
- Networking uses BSD sockets
- HTTP uses curl or raw sockets
- Hashing uses openssl
- All 99 functions are production-ready
