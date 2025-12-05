# Wyn Quick Start Guide

## Installation (30 seconds)

```bash
git clone https://github.com/wyn-lang/wyn.git
cd wyn
./install.sh
```

This will:
1. Build the compiler
2. Run tests
3. Install to `/usr/local/bin/wyn`

## Your First Program (1 minute)

Create `hello.wyn`:
```wyn
fn main() {
    io.print_str("Hello, World!")
    io.print_newline()
}
```

Compile and run:
```bash
wyn -o hello hello.wyn
./hello
```

## Examples (5 minutes)

### CLI Tool
```wyn
fn main() {
    io.print_str("Enter your name: ")
    let name = read_line()
    io.print_str("Hello, ")
    io.print_str(name)
    io.print_newline()
}
```

### HTTP Server
```wyn
fn main() {
    let server = tcp_listen(8080)
    io.print_str("Server running on :8080")
    io.print_newline()
    
    while true {
        let client = tcp_accept(server)
        tcp_send(client, "HTTP/1.1 200 OK\r\n\r\nHello!")
        tcp_close(client)
    }
}
```

### Concurrent Program
```wyn
fn worker(id: int) {
    io.print_str("Worker ")
    print(id)
    io.print_newline()
}

fn main() {
    spawn { worker(1) }
    spawn { worker(2) }
    spawn { worker(3) }
}
```

### GUI Application (macOS)
```wyn
fn main() {
    let win = gui.create_window("My App", 800, 600)
    gui.show_window(win)
    gui.draw_rect(win, 100, 100, 200, 150, 16711680)
    gui.draw_text(win, 120, 180, "Hello, GUI!")
    
    # Poll events for 5 seconds
    let i = 0
    while i < 500 {
        gui.poll_events()
        sleep_ms(10)
        i = i + 1
    }
}
```

## Next Steps

- Read [Language Spec](2-language-spec.md)
- Check [Examples](../examples/)
- See [Benchmarks](../benchmarks/FINAL_RESULTS.md)
- Join community (GitHub)

## Common Commands

```bash
# Compile with optimizations
wyn --stage1-tc --stage1-opt -o output input.wyn

# Run tests
make test

# Build examples
wyn -o calc examples/calculator.wyn
```

## Getting Help

- Documentation: `docs/`
- Examples: `examples/`
- GitHub: https://github.com/wyn-lang/wyn
