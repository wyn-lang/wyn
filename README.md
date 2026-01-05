# Wyn

A simple compiled programming language.

## Install

```bash
make
./install.sh
```

## Usage

```bash
wyn program.wyn          # compile
wyn run program.wyn      # compile and run
wyn test                 # run tests
```

## Example

```wyn
fn fibonacci(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

fn main() -> int {
    for var i = 0; i < 10; i += 1 {
        print(fibonacci(i))
    }
    return 0
}
```

## Features

- Variables and functions
- Structs, enums, arrays
- Pattern matching
- Type checking
- Fast compilation

## Documentation

See [wynlang.com](https://wynlang.com) for complete documentation and guides.

## License

MIT
